//
// Created by Rob Pieké on 30/06/2016.
//

#pragma once

#include <RixSampleFilter.h>
#include "SampleFilterInstance.h"

class PathtracerIntegrator : public RixSampleFilter
{

public:
	virtual int Init(RixContext &ctx, char const *pluginPath) override;

	virtual void Finalize(RixContext &ctx) override;

	virtual int CreateInstanceData(RixContext &ctx, char const *handle, RixParameterList const *params, InstanceData *instance) override;

	virtual void Filter(RixSampleFilterContext &fCtx, RtConstPointer instance) override;

	virtual RixSCParamInfo const *GetParamTable() override;

private:
	struct iData
	{
		SampleFilterInstance camera;
		std::vector<SampleFilterInstance> world;
		RtFloat rayOffset;
		int maxPathLength;
		RixChannelId ciChannel;
		RixChannelId zChannel;
		RixChannelId nChannel;
		RixChannelId pChannel;
		RixChannelId rayOChannel;
		RixChannelId rayDChannel;
		RixChannelId rayTChannel;
		RixChannelId matChannel;
		RixChannelId maskChannel;
		RixChannelId albedoChannel;
	};
};
