//
// Created by Rob Pieké on 15/07/2016.
//

#include "Light.h"

int Light::Init(RixContext &ctx, char const *pluginPath)
{
	return 0;
}

RixSCParamInfo const *Light::GetParamTable()
{
	static RixSCParamInfo s_ptable[] =
	{
		RixSCParamInfo("emission", k_RixSCColor),
		RixSCParamInfo()
	};
	return &s_ptable[0];
}

void Light::Finalize(RixContext &ctx)
{

}

int Light::CreateInstanceData(RixContext &ctx, char const *handle, RixParameterList const *params, RixShadingPlugin::InstanceData *instance)
{
	instance->datalen = sizeof(iData);
	instance->data = malloc(instance->datalen);
	instance->freefunc = free;
	iData *data = new (instance->data) iData;

	int paramId;
	data->emission = RtColorRGB(1.f);
	if(0 == params->GetParamId("emission", &paramId)) params->EvalParam(paramId, 0, &data->emission);

	RixRenderState *renderState = static_cast<RixRenderState*>(ctx.GetRixInterface(k_RixRenderState));
	RixRenderState::FrameInfo frameInfo;
	renderState->GetFrameInfo(&frameInfo);
	RixIntegratorEnvironment const *iEnv = frameInfo.integratorEnv;

	data->maskChannel = iEnv->GetDisplayChannel("mask")->id;
	data->nChannel = iEnv->GetDisplayChannel("N")->id;
	data->zChannel = iEnv->GetDisplayChannel("z")->id;
	data->rayDChannel = iEnv->GetDisplayChannel("rayD")->id;
	data->albedoChannel = iEnv->GetDisplayChannel("albedo")->id;

	return 0;
}

void Light::Filter(RixSampleFilterContext &fCtx, RtConstPointer instance)
{
	const iData *data = reinterpret_cast<const iData*>(instance);
	for(int i = 0; i < fCtx.numSamples; ++i)
	{
		float mask;
		fCtx.Read(data->maskChannel, i, mask);
		if(mask == 0) continue;
		RtColorRGB cTmp;
		fCtx.Read(data->nChannel, i, cTmp);
		RtNormal3 N(cTmp);
		fCtx.Read(data->rayDChannel, i, cTmp);
		RtVector3 D(cTmp);
		RtFloat z;
		fCtx.Read(data->zChannel, i, z);
		fCtx.Write(data->rayDChannel, i, RtColorRGB(0.f));
		fCtx.Write(data->albedoChannel, i, data->emission);
	}
}

RIX_SAMPLEFILTERCREATE
{
	return new Light();
}

RIX_SAMPLEFILTERDESTROY
{
	delete reinterpret_cast<Light*>(filter);
}
