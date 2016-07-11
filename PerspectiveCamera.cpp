//
// Created by Rob Pieké on 30/06/2016.
//

#include "PerspectiveCamera.h"


int PerspectiveCamera::Init(RixContext &ctx, char const *pluginPath)
{
	return 0;
}

RixSCParamInfo const *PerspectiveCamera::GetParamTable()
{
	static RixSCParamInfo s_ptable[] =
	{
		RixSCParamInfo("from", k_RixSCPoint),
		RixSCParamInfo("to", k_RixSCPoint),
		RixSCParamInfo("fov", k_RixSCFloat),
		RixSCParamInfo("focusDist", k_RixSCFloat),
		RixSCParamInfo("aperture", k_RixSCFloat),
		RixSCParamInfo()
	};
	return &s_ptable[0];
}

void PerspectiveCamera::Finalize(RixContext &ctx)
{

}

int PerspectiveCamera::CreateInstanceData(RixContext &ctx, char const *handle, RixParameterList const *params, RixShadingPlugin::InstanceData *instance)
{
	instance->datalen = sizeof(iData);
	instance->data = malloc(instance->datalen);
	instance->freefunc = free;
	iData *data = new (instance->data) iData;

	data->from = RtPoint3(0, 0, -10);
	data->to = RtPoint3(0, 0, 0);
	data->focusDist = 1.f;
	data->aperture = 0.f;

	int paramId;
	if(0 == params->GetParamId("from", &paramId)) params->EvalParam(paramId, 0, &data->from);
	if(0 == params->GetParamId("to", &paramId)) params->EvalParam(paramId, 0, &data->to);
	if(0 == params->GetParamId("focusDist", &paramId)) params->EvalParam(paramId, 0, &data->focusDist);
	if(0 == params->GetParamId("aperture", &paramId)) params->EvalParam(paramId, 0, &data->aperture);
	float fov = 45.f;
	if(0 == params->GetParamId("fov", &paramId)) params->EvalParam(paramId, 0, &fov);

	float height = 2.f * tanf(fov / 2.f / 2.f * M_PI / 180.f);
	RtVector3 view = NormalizeCopy(data->to - data->from);
	data->horiz = NormalizeCopy(RtVector3(0.f, 1.f, 0.f).Cross(view)) * height * data->focusDist;
	data->vert = NormalizeCopy(view.Cross(data->horiz)) * height * data->focusDist;
	data->target = view * data->focusDist;

	RixRenderState *renderState = static_cast<RixRenderState*>(ctx.GetRixInterface(k_RixRenderState));
	RixRenderState::FrameInfo frameInfo;
	renderState->GetFrameInfo(&frameInfo);
	RixIntegratorEnvironment const *iEnv = frameInfo.integratorEnv;

	data->rayOChannel = iEnv->GetDisplayChannel("rayO")->id;
	data->rayDChannel = iEnv->GetDisplayChannel("rayD")->id;

	return 0;
}

void PerspectiveCamera::Filter(RixSampleFilterContext &fCtx, RtConstPointer instance)
{
	const iData *data = reinterpret_cast<const iData*>(instance);
	for(int i = 0; i < fCtx.numSamples; ++i)
	{
		RtVector3 offset =
			NormalizeCopy(data->horiz) * (float(rand()) / RAND_MAX - 0.5) * data->aperture +
			NormalizeCopy(data->vert) * (float(rand()) / RAND_MAX - 0.5) * data->aperture;
		RtPoint3 o = data->from + offset;
		fCtx.Write(data->rayOChannel, i, *(RtColorRGB*)&(o.x));
		RtVector3 d = NormalizeCopy(
				data->target +
				data->horiz * fCtx.screen[i].x +
				data->vert * fCtx.screen[i].y -
				offset);

		fCtx.Write(data->rayDChannel, i, *(RtColorRGB*)&d.x);
	}
}

RIX_SAMPLEFILTERCREATE
{
	return new PerspectiveCamera();
}

RIX_SAMPLEFILTERDESTROY
{
	delete reinterpret_cast<PerspectiveCamera*>(filter);
}
