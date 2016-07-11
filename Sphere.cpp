//
// Created by Rob Pieké on 30/06/2016.
//

#include "Sphere.h"


int Sphere::Init(RixContext &ctx, char const *pluginPath)
{
	return 0;
}

RixSCParamInfo const *Sphere::GetParamTable()
{
	static RixSCParamInfo s_ptable[] =
	{
		RixSCParamInfo("centre", k_RixSCPoint),
		RixSCParamInfo("radius", k_RixSCFloat),
		RixSCParamInfo("material", k_RixSCSampleFilter),
		RixSCParamInfo()
	};
	return &s_ptable[0];
}

void Sphere::Finalize(RixContext &ctx)
{

}


int Sphere::CreateInstanceData(RixContext &ctx, char const *handle, RixParameterList const *params, RixShadingPlugin::InstanceData *instance)
{
	instance->datalen = sizeof(iData);
	instance->data = malloc(instance->datalen);
	instance->freefunc = free;
	iData *data = new (instance->data) iData;

	data->centre = RtPoint3(0.f);
	data->radius = 1.f;
	data->material.f = nullptr;

	int paramId;
	if(0 == params->GetParamId("centre", &paramId)) params->EvalParam(paramId, 0, &data->centre);
	if(0 == params->GetParamId("radius", &paramId)) params->EvalParam(paramId, 0, &data->radius);
	if(0 == params->GetParamId("material", &paramId)) params->EvalParam(paramId, 0, &data->material.f, &data->material.i);

	RixRenderState *renderState = static_cast<RixRenderState*>(ctx.GetRixInterface(k_RixRenderState));
	RixRenderState::FrameInfo frameInfo;
	renderState->GetFrameInfo(&frameInfo);
	RixIntegratorEnvironment const *iEnv = frameInfo.integratorEnv;

	data->zChannel = iEnv->GetDisplayChannel("z")->id;
	data->pChannel = iEnv->GetDisplayChannel("P")->id;
	data->nChannel = iEnv->GetDisplayChannel("N")->id;
	data->rayOChannel = iEnv->GetDisplayChannel("rayO")->id;
	data->rayDChannel = iEnv->GetDisplayChannel("rayD")->id;
	data->matChannel = iEnv->GetDisplayChannel("material")->id;

	return 0;
}

void Sphere::Filter(RixSampleFilterContext &fCtx, RtConstPointer instance)
{
	const iData *data = reinterpret_cast<const iData*>(instance);

	RtColorRGB cTmp;
	for(int i = 0; i < fCtx.numSamples; ++i)
	{
		fCtx.Read(data->rayOChannel, i, cTmp);
		RtPoint3 rayO(cTmp);
		RtVector3 co = rayO - data->centre;
		fCtx.Read(data->rayDChannel, i, cTmp);
		RtVector3 rayD(cTmp);
		RtFloat a = rayD.LengthSq();
		RtFloat b = 2 * RtVector3(cTmp).Dot(co);
		RtFloat c = co.Dot(co) - data->radius * data->radius;
		RtFloat d = b * b - 4 * a * c;

		if(d < 0) continue;

		float t = (-b - sqrtf(d)) / (2 * a);
		if(t < 0) continue;

		float curZ;
		fCtx.Read(data->zChannel, i, curZ);
		if(t > curZ) continue;

		RtPoint3 P = rayO + rayD * t;
		RtNormal3 N = (P - data->centre) / data->radius;

		fCtx.Write(data->zChannel, i, t);
		fCtx.Write(data->pChannel, i, *(RtColorRGB*)(&P.x));
		fCtx.Write(data->nChannel, i, *(RtColorRGB*)(&N.x));

		// Ugly/interesting passing of pointers encoded in floating-point buffers
		RtColorRGB material;
		*reinterpret_cast<const SampleFilterInstance**>(&material) = &data->material;
		fCtx.Write(data->matChannel, i, material);
	}
}

RIX_SAMPLEFILTERCREATE
{
	return new Sphere();
}

RIX_SAMPLEFILTERDESTROY
{
	delete reinterpret_cast<Sphere*>(filter);
}

