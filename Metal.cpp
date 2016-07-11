//
// Created by Rob Pieké on 07/07/2016.
//

#include "Metal.h"

int Metal::Init(RixContext &ctx, char const *pluginPath)
{
	return 0;
}

RixSCParamInfo const *Metal::GetParamTable()
{
	static RixSCParamInfo s_ptable[] =
			{
					RixSCParamInfo("albedo", k_RixSCColor),
					RixSCParamInfo()
			};
	return &s_ptable[0];
}

void Metal::Finalize(RixContext &ctx)
{

}

int Metal::CreateInstanceData(RixContext &ctx, char const *handle, RixParameterList const *params, RixShadingPlugin::InstanceData *instance)
{
	instance->datalen = sizeof(iData);
	instance->data = malloc(instance->datalen);
	instance->freefunc = free;
	iData *data = new (instance->data) iData;

	int paramId;
	data->albedo = RtColorRGB(0.5f);
	if(0 == params->GetParamId("albedo", &paramId)) params->EvalParam(paramId, 0, &data->albedo);

	RixRenderState *renderState = static_cast<RixRenderState*>(ctx.GetRixInterface(k_RixRenderState));
	RixRenderState::FrameInfo frameInfo;
	renderState->GetFrameInfo(&frameInfo);
	RixIntegratorEnvironment const *iEnv = frameInfo.integratorEnv;

	data->maskChannel = iEnv->GetDisplayChannel("mask")->id;
	data->nChannel = iEnv->GetDisplayChannel("N")->id;
	data->rayDChannel = iEnv->GetDisplayChannel("rayD")->id;
	data->albedoChannel = iEnv->GetDisplayChannel("albedo")->id;

	return 0;
}

void Metal::Filter(RixSampleFilterContext &fCtx, RtConstPointer instance)
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
		D = D - 2 * D.Dot(N) * N;
		fCtx.Write(data->rayDChannel, i, *(RtColorRGB*)(&D.x));
		fCtx.Write(data->albedoChannel, i, data->albedo);
	}
}

RIX_SAMPLEFILTERCREATE
{
	return new Metal();
}

RIX_SAMPLEFILTERDESTROY
{
	delete reinterpret_cast<Metal*>(filter);
}
