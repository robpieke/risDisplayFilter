//
// Created by Rob Pieké on 30/06/2016.
//

#include <set>
#include "PathtracerIntegrator.h"


int PathtracerIntegrator::Init(RixContext &ctx, char const *pluginPath)
{
	return 0;
}

void PathtracerIntegrator::Finalize(RixContext &ctx)
{

}

int PathtracerIntegrator::CreateInstanceData(RixContext &ctx, char const *handle, RixParameterList const *params, RixShadingPlugin::InstanceData *instance)
{
	RixMessages *msg = reinterpret_cast<RixMessages*>(ctx.GetRixInterface( k_RixMessages ) );

	instance->datalen = sizeof(iData);
	instance->data = malloc(instance->datalen);
	instance->freefunc = free;
	iData *data = new (instance->data) iData;

	data->camera.f = nullptr;
	data->rayOffset = 0.01f;
	data->maxPathLength = 5;
	data->skyColour = RtColorRGB(1.f);

	int paramId;
	if(0 == params->GetParamId("camera", &paramId))
	{
		params->EvalParam(paramId, 0, &data->camera.f, &data->camera.i);
	}
	else
	{
		msg->Info("Couldn't find the 'camera' parameter on 'PathtracerIntegrator', using render camera");
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
	if(0 == params->GetParamId("maxPathLength", &paramId)) params->EvalParam(paramId, 0, &data->maxPathLength);
	if(0 == params->GetParamId("rayOffset", &paramId)) params->EvalParam(paramId, 0, &data->rayOffset);
	if(0 == params->GetParamId("skyColor", &paramId)) params->EvalParam(paramId, 0, &data->skyColour);

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
	data->matChannel = iEnv->GetDisplayChannel("material")->id;
	data->maskChannel = iEnv->GetDisplayChannel("mask")->id;
	data->albedoChannel = iEnv->GetDisplayChannel("albedo")->id;

	return 0;
}

void PathtracerIntegrator::Filter(RixSampleFilterContext &fCtx, RtConstPointer instance)
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
		fCtx.Write(data->ciChannel, i, RtColorRGB(1.f));
		fCtx.Write(data->zChannel, i, FLT_MAX);
	}

	// Adjust/reset up the camera rays
	//
	if(data->camera.f)
	{
		RtConstPointer instanceData;
		fCtx.IsEnabled(data->camera.i, &instanceData);
		data->camera.f->Filter(fCtx, instanceData);
	}

	for(int pathSegment = 0; pathSegment < data->maxPathLength; ++pathSegment)
	{
		// Trace the scene
		//
		for(auto geo : data->world)
		{
			RtConstPointer instanceData;
			fCtx.IsEnabled(geo.i, &instanceData);
			geo.f->Filter(fCtx, instanceData);
		}

		// Collect an inventory of all the hit materials
		//
		std::set<SampleFilterInstance*> hitMaterials;
		for(int i = 0; i < fCtx.numSamples; ++i)
		{
			RtFloat z;
			fCtx.Read(data->zChannel, i, z);
			// Ray miss or no ray
			if(z == FLT_MAX || z == -1) continue;
			RtColorRGB cTmp;
			fCtx.Read(data->matChannel, i, cTmp);
			hitMaterials.insert(*reinterpret_cast<SampleFilterInstance**>(&cTmp));
		}

		// Iterate over the materials, mask the buffer, and shade
		//
		for(auto curMaterial : hitMaterials)
		{
			for(int i = 0; i < fCtx.numSamples; ++i)
			{
				fCtx.Write(data->maskChannel, i, 0.f);
				RtFloat z;
				fCtx.Read(data->zChannel, i, z);
				// Ray miss or no ray
				if(z == FLT_MAX || z == -1) continue;
				RtColorRGB cTmp;
				fCtx.Read(data->matChannel, i, cTmp);
				SampleFilterInstance *material = *reinterpret_cast<SampleFilterInstance**>(&cTmp);
				if(material == curMaterial) fCtx.Write(data->maskChannel, i, 1.f);
			}
			RtConstPointer instanceData;
			fCtx.IsEnabled(curMaterial->i, &instanceData);
			curMaterial->f->Filter(fCtx, instanceData);
		}

		// Multiply in the resulting albedo
		//
		for(int i = 0; i < fCtx.numSamples; ++i)
		{
			RtFloat z;
			fCtx.Read(data->zChannel, i, z);
			// No ray
			if(z == -1.f) continue;
			RtColorRGB Ci;
			fCtx.Read(data->ciChannel, i, Ci);
			RtColorRGB d;
			fCtx.Read(data->rayDChannel, i, d);
			// Ray miss
			if(z == FLT_MAX && !d.IsZero())
			{
				fCtx.Write(data->ciChannel, i, Ci * data->skyColour);
				fCtx.Write(data->rayDChannel, i, RtColorRGB(0.f));
				fCtx.Write(data->zChannel, i, -1.f);
				continue;
			}
			RtColorRGB albedo;
			fCtx.Read(data->albedoChannel, i, albedo);
			fCtx.Write(data->ciChannel, i, Ci * albedo);
		}

		// Set up next rays
		//
		for(int i = 0; i < fCtx.numSamples; ++i)
		{
			RtColorRGB P, D;
			fCtx.Read(data->pChannel, i, P);
			fCtx.Read(data->rayDChannel, i, D);
			if(D.IsZero()) fCtx.Write(data->zChannel, i, -1.f);
			else fCtx.Write(data->zChannel, i, FLT_MAX);
			fCtx.Write(data->rayOChannel, i, P + D * data->rayOffset);
		}

	}

	// "Shade to black" rays that never escaped
	//
	for(int i = 0; i < fCtx.numSamples; ++i)
	{
		RtColorRGB D;
		RtFloat z;
		fCtx.Read(data->rayDChannel, i, D);
		fCtx.Read(data->zChannel, i, z);
		if(!D.IsZero() && z != -1) fCtx.Write(data->ciChannel, i, RtColorRGB(0.f));
	}
}

RixSCParamInfo const *PathtracerIntegrator::GetParamTable()
{
	static RixSCParamInfo s_ptable[] =
	{
		RixSCParamInfo("camera", k_RixSCSampleFilter),
		RixSCParamInfo("world", k_RixSCSampleFilter, k_RixSCInput, 0),
		RixSCParamInfo("rayOffset", k_RixSCFloat),
		RixSCParamInfo("maxPathLength", k_RixSCInteger),
		RixSCParamInfo("skyColor", k_RixSCColor),
		RixSCParamInfo()
	};
	return &s_ptable[0];
}

RIX_SAMPLEFILTERCREATE
{
	return new PathtracerIntegrator();
}

RIX_SAMPLEFILTERDESTROY
{
	delete reinterpret_cast<PathtracerIntegrator*>(filter);
}
