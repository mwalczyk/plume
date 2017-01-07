#pragma once
#pragma once

#include <memory>
#include <vector>
#include <fstream>
#include <string>

#include "Platform.h"
#include "Device.h"

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
			
			float a;
			float b;
		};

		//! Factory method for returning a new PipelineRef.
		static PipelineRef create(const DeviceRef &tDevice, const Options &tOptions = Options()) 
		{ 
			return std::make_shared<Pipeline>(tDevice, tOptions);
		}

		Pipeline(const DeviceRef &tDevice, const Options &tOptions = Options());
		~Pipeline();

		inline VkPipeline getHandle() const { return mPipelineHandle; }

	private:

		void createRenderPass();
		void createShaderModule(const std::vector<char> &tSource, VkShaderModule *tShaderModule);

		VkPipeline mPipelineHandle;
		VkPipelineLayout mPipelineLayoutHandle;
		VkRenderPass mRenderPassHandle;

		DeviceRef mDevice;

	};

} // namespace vk