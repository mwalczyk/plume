#pragma once
#pragma once

#include <memory>
#include <vector>
#include <map>
#include <fstream>
#include <string>

#include "spirv_glsl.hpp"
#include <utility>

#include "Platform.h"
#include "Device.h"
#include "RenderPass.h"

namespace vk
{

	class ShaderModule;
	using ShaderModuleRef = std::shared_ptr<ShaderModule>;

	class ShaderModule
	{
		
	public:

		struct PushConstantsBlock
		{
			uint32_t layoutLocation;
			uint32_t totalSize;
			std::string name;
		};

		struct PushConstantsMember
		{
			/*PushConstantsMember(uint32_t tIndex, uint32_t tSize, uint32_t tOffset, const std::string &tName) :
				index(tIndex),
				size(tSize),
				offset(tOffset),
				name(tName)
			{
			}*/

			uint32_t index;
			uint32_t size;
			uint32_t offset;
			std::string name;
		};

		struct PushConstantsBlockOrdering
		{
			bool operator() (const PushConstantsBlock& lhs, const PushConstantsBlock& rhs) const
			{
				return lhs.layoutLocation < rhs.layoutLocation;
			}
		};

		static ShaderModuleRef create(const DeviceRef &tDevice, const std::string &tFilePath)
		{
			return std::make_shared<ShaderModule>(tDevice, tFilePath);
		}

		ShaderModule(const DeviceRef &tDevice, const std::string &tFilePath);
		~ShaderModule();

		inline VkShaderModule getHandle() const { return mShaderModuleHandle; }

	private:

		void performReflection();

		DeviceRef mDevice;

		VkShaderModule mShaderModuleHandle;

		std::vector<uint32_t> mShaderCode;
		std::vector<std::string> mEntryPoints;
		std::map<PushConstantsBlock, std::vector<PushConstantsMember>, PushConstantsBlockOrdering> mPushConstantsMapping;

	};

	class Pipeline;
	using PipelineRef = std::shared_ptr<Pipeline>;

	class Pipeline
	{

	public:

		struct Options
		{
			Options();
			
			Options& pushConstantRanges(const std::vector<VkPushConstantRange>& tPushConstantRanges) { mPushConstantRanges = tPushConstantRanges; return *this; }
			Options& vertexInputBindingDescriptions(const std::vector<VkVertexInputBindingDescription>& tVertexInputBindingDescriptions) { mVertexInputBindingDescriptions = tVertexInputBindingDescriptions; return *this; }
			Options& vertexInputAttributeDescriptions(const std::vector<VkVertexInputAttributeDescription>& tVertexInputAttributeDescriptions) { mVertexInputAttributeDescriptions = tVertexInputAttributeDescriptions; return *this; }

			std::vector<VkPushConstantRange> mPushConstantRanges;
			std::vector<VkVertexInputBindingDescription> mVertexInputBindingDescriptions;
			std::vector<VkVertexInputAttributeDescription> mVertexInputAttributeDescriptions;
			ShaderModuleRef mVertexShader;
			ShaderModuleRef mTessellationControlShader;
			ShaderModuleRef mTessellationEvaluationShader;
			ShaderModuleRef mGeometryShader;
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

		VkPipeline mPipelineHandle;
		VkPipelineLayout mPipelineLayoutHandle;
		std::vector<VkPushConstantRange> mPushConstantRanges;

		DeviceRef mDevice;
		RenderPassRef mRenderPass;

	};

} // namespace vk