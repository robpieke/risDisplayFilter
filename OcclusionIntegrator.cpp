//
// Created by Rob Pieké on 30/06/2016.
//

#include "OcclusionIntegrator.h"

int OcclusionIntegrator::Init(RixContext &ctx, char const *pluginPath)
{
	return 0;
}

void OcclusionIntegrator::Finalize(RixContext &ctx)
{

}

int OcclusionIntegrator::CreateInstanceData(RixContext &ctx, char const *handle, RixParameterList const *params, RixShadingPlugin::InstanceData *instance)
{
	RixMessages *msg = reinterpret_cast<RixMessages*>(ctx.GetRixInterface( k_RixMessages ) );

	instance->datalen = sizeof(iData);
	instance->data = malloc(instance->datalen);
	instance->freefunc = free;
	iData *data = new (instance->data) iData;

	data->camera.f = nullptr;

	int paramId;
	if(0 == params->GetParamId("camera", &paramId))
	{
		params->EvalParam(paramId, 0, &data->camera.f, &data->camera.i);
	}
	else
	{
		msg->Info("Couldn't find the 'camera' parameter on 'OcclusionIntegrator', using render camera");
	}
	if(0 == params->GetParamId("world", &paramId))
	{
		RixSCType pType;
		bool isConnected;
		int arrayLen;
		params->GetParamInfo(paramId, &pType, &isConnected, &arrayLen);
		data->world.resize(arrayLen);
		for(int i = 0; i < arrayLen; ++i)
		{
			params->EvalParam(paramId, i, &data->world[i].f, &data->world[i].i);
		}
	}

	data->rayOffset = 0.01f;
	if(0 == params->GetParamId("rayOffset", &paramId)) params->EvalParam(paramId, 0, &data->rayOffset);

	RixRenderState *renderState = (RixRenderState*) ctx.GetRixInterface(k_RixRenderState);
	RixRenderState::FrameInfo frameInfo;
	renderState->GetFrameInfo(&frameInfo);
	RixIntegratorEnvironment const *iEnv = frameInfo.integratorEnv;

	data->ciChannel = iEnv->GetDisplayChannel("Ci")->id;
	data->zChannel = iEnv->GetDisplayChannel("z")->id;
	data->pChannel = iEnv->GetDisplayChannel("P")->id;
	data->nChannel = iEnv->GetDisplayChannel("N")->id;
	data->rayOChannel = iEnv->GetDisplayChannel("rayO")->id;
	data->rayDChannel = iEnv->GetDisplayChannel("rayD")->id;
	data->rayTChannel = iEnv->GetDisplayChannel("rayT")->id;

	return 0;
}

void OcclusionIntegrator::Filter(RixSampleFilterContext &fCtx, RtConstPointer instance)
{
	const iData *data = reinterpret_cast<const iData*>(instance);

	// Copy&transform camera rays from PRMan
	//
	RixTransform *xform = (RixTransform*) fCtx.GetRixInterface(k_RixTransform);
	RtMatrix4x4 mtx;
	xform->TransformMatrix("camera", "world", 0.f, mtx.m);
	for(int i = 0; i < fCtx.numSamples; ++i)
	{
		RtPoint3 o = mtx.pTransform(fCtx.rays[i].origin);
		RtVector3 d = mtx.vTransform(fCtx.rays[i].direction);
		fCtx.Write(data->rayOChannel, i, *(RtColorRGB*)(&o.x));
		fCtx.Write(data->rayDChannel, i, *(RtColorRGB*)(&d.x));
		fCtx.Write(data->rayTChannel, i, fCtx.shutter[i]);
	}

	// Adjust/reset up the camera rays
	//
	if(data->camera.f)
	{
		RtConstPointer instanceData;
		fCtx.IsEnabled(data->camera.i, &instanceData);
		data->camera.f->Filter(fCtx, instanceData);
	}

	// Trace the scene to get primary hits, first resetting the depth
	//
	for(int i = 0; i < fCtx.numSamples; ++i) fCtx.Write(data->zChannel, i, FLT_MAX);
	for(auto geo : data->world)
	{
		RtConstPointer instanceData;
		fCtx.IsEnabled(geo.i, &instanceData);
		geo.f->Filter(fCtx, instanceData);
	}

	// Trace the scene to get occlusion hits, first resetting the depth and setting up the ray directions
	//
	for(int i = 0; i < fCtx.numSamples; ++i)
	{
		RtColorRGB cTmp;
		fCtx.Read(data->nChannel, i, cTmp);
		RtVector3 n(cTmp);
		RtVector3 d;
		do
		{
			d = RtVector3(
					float(rand()) / RAND_MAX * 2 - 1,
					float(rand()) / RAND_MAX * 2 - 1,
					float(rand()) / RAND_MAX * 2 - 1);
		} while(d.LengthSq() > 1.f);
		d += n;
		d.Normalize();
		fCtx.Write(data->rayDChannel, i, *(RtColorRGB *) (&d.x));
		fCtx.Read(data->pChannel, i, cTmp);
		fCtx.Write(data->rayOChannel, i, cTmp + RtColorRGB(d.x, d.y, d.z) * data->rayOffset);
		fCtx.Write(data->zChannel, i, FLT_MAX);
	}
	for(auto geo : data->world)
	{
		RtConstPointer instanceData;
		fCtx.IsEnabled(geo.i, &instanceData);
		geo.f->Filter(fCtx, instanceData);
	}

	// "Shade" and "splat"
	//
	for(int i = 0; i < fCtx.numSamples; ++i)
	{
		RtColorRGB Ci;
		RtFloat z;
		fCtx.Read(data->ciChannel, i, Ci);
		fCtx.Read(data->zChannel, i, z);
		fCtx.Write(data->ciChannel, i, z == FLT_MAX ? RtColorRGB(1.f) : RtColorRGB(0.f));
	}
}

RixSCParamInfo const *OcclusionIntegrator::GetParamTable()
{
	static RixSCParamInfo s_ptable[] =
	{
		RixSCParamInfo("camera", k_RixSCSampleFilter),
		RixSCParamInfo("world", k_RixSCSampleFilter, k_RixSCInput, 0),
		RixSCParamInfo("rayOffset", k_RixSCFloat),
		RixSCParamInfo()
	};
	return &s_ptable[0];
}

RIX_SAMPLEFILTERCREATE
{
	return new OcclusionIntegrator();
}

RIX_SAMPLEFILTERDESTROY
{
	delete reinterpret_cast<OcclusionIntegrator*>(filter);
}
