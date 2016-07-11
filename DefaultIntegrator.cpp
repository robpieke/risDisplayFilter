//
// Created by Rob Pieké on 30/06/2016.
//

#include "DefaultIntegrator.h"


int DefaultIntegrator::Init(RixContext &ctx, char const *pluginPath)
{
	return 0;
}

void DefaultIntegrator::Finalize(RixContext &ctx)
{

}

int DefaultIntegrator::CreateInstanceData(RixContext &ctx, char const *handle, RixParameterList const *params, RixShadingPlugin::InstanceData *instance)
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
		msg->Info("Couldn't find the 'camera' parameter on 'DefaultIntegrator', using render camera");
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

void DefaultIntegrator::Filter(RixSampleFilterContext &fCtx, RtConstPointer instance)
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

	// Trace the scene, first resetting the depth
	//
	for(int i = 0; i < fCtx.numSamples; ++i) fCtx.Write(data->zChannel, i, FLT_MAX);
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
		RtColorRGB N, rayD;
		fCtx.Read(data->nChannel, i, N);
		fCtx.Read(data->rayDChannel, i, rayD);
		float z;
		fCtx.Read(data->zChannel, i, z);
		RtFloat facing = RtNormal3(N).AbsDot(RtVector3(rayD));
		fCtx.Write(data->ciChannel, i, RtColorRGB(facing));
	}
}

RixSCParamInfo const *DefaultIntegrator::GetParamTable()
{
	static RixSCParamInfo s_ptable[] =
	{
		RixSCParamInfo("camera", k_RixSCSampleFilter),
		RixSCParamInfo("world", k_RixSCSampleFilter, k_RixSCInput, 0),
		RixSCParamInfo()
	};
	return &s_ptable[0];
}

RIX_SAMPLEFILTERCREATE
{
	return new DefaultIntegrator();
}

RIX_SAMPLEFILTERDESTROY
{
	delete reinterpret_cast<DefaultIntegrator*>(filter);
}
