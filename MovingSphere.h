//
// Created by Rob Pieké on 07/07/2016.
//

#pragma once

#include <RixSampleFilter.h>
#include "SampleFilterInstance.h"

class MovingSphere : public RixSampleFilter
{
public:
	virtual int Init(RixContext &ctx, char const *pluginPath) override;

	virtual RixSCParamInfo const *GetParamTable() override;

	virtual void Finalize(RixContext &ctx) override;

	virtual int CreateInstanceData(RixContext &ctx, char const *handle, RixParameterList const *params, InstanceData *instance) override;

	virtual void Filter(RixSampleFilterContext &fCtx, RtConstPointer instance) override;

private:
	struct iData
	{
		RtPoint3 centre0, centre1;
		RtFloat radius;
		SampleFilterInstance material;
		RixChannelId zChannel;
		RixChannelId nChannel;
		RixChannelId pChannel;
		RixChannelId rayOChannel;
		RixChannelId rayDChannel;
		RixChannelId rayTChannel;
		RixChannelId matChannel;
	};


};
