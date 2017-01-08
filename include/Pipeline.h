#pragma once
#pragma once

#include <memory>
#include <vector>
#include <fstream>
#include <string>

#include "Platform.h"
#include "Device.h"
#include "RenderPass.h"

namespace vk
{

	class Pipeline;
	using PipelineRef = std::shared_ptr<Pipeline>;

	class Pipeline
	{

	public:

		struct Options
		{
			Options();
			
			Options& pushConstantRanges(const std::vector<VkPushConstantRange>& tPushConstantRanges) { mPushConstantRanges = tPushConstantRanges; return *this; }

			std::vector<VkPushConstantRange> mPushConstantRanges;
		};

		//! Factory method for returning a new PipelineRef.
		static PipelineRef create(const DeviceRef &tDevice, const RenderPassRef &tRenderPass, const Options &tOptions = Options()) 
		{ 
			return std::make_shared<Pipeline>(tDevice, tRenderPass, tOptions);
		}

		Pipeline(const DeviceRef &tDevice, const RenderPassRef &tRenderPass, const Options &tOptions = Options());
		~Pipeline();

		inline VkPipeline getHandle() const { return mPipelineHandle; }
		inline VkPipelineLayout getPipelineLayoutHandle() const { return mPipelineLayoutHandle; }

	private:

		void createShaderModule(const std::vector<char> &tSource, VkShaderModule *tShaderModule);

		VkPipeline mPipelineHandle;
		VkPipelineLayout mPipelineLayoutHandle;
		std::vector<VkPushConstantRange> mPushConstantRanges;

		DeviceRef mDevice;
		RenderPassRef mRenderPass;

	};

} // namespace vk