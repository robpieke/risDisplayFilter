//
// Created by Rob Pieké on 15/07/2016.
//

#pragma once

#include <RixSampleFilter.h>

class Light : public RixSampleFilter
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
		RtColorRGB emission;
		RixChannelId maskChannel;
		RixChannelId nChannel;
		RixChannelId zChannel;
		RixChannelId rayDChannel;
		RixChannelId albedoChannel;
	};

};
