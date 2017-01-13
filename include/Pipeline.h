#pragma once
#pragma once

#include <memory>
#include <vector>
#include <map>
#include <iterator>
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

		using PushConstantsMapping = std::map<PushConstantsBlock, std::vector<PushConstantsMember>, PushConstantsBlockOrdering>;

		static ShaderModuleRef create(const DeviceRef &tDevice, const std::string &tFilePath)
		{
			return std::make_shared<ShaderModule>(tDevice, tFilePath);
		}

		ShaderModule(const DeviceRef &tDevice, const std::string &tFilePath);
		~ShaderModule();

		inline VkShaderModule getHandle() const { return mShaderModuleHandle; }
		inline const std::vector<uint32_t>& getShaderCode() const { return mShaderCode; }
		inline const std::vector<std::string>& getEntryPoints() const { return mEntryPoints; }
		inline const PushConstantsMapping& getPushConstantsMapping() const { return mPushConstantsMapping; }

	private:

		void performReflection();

		DeviceRef mDevice;

		VkShaderModule mShaderModuleHandle;

		std::vector<uint32_t> mShaderCode;
		std::vector<std::string> mEntryPoints;
		PushConstantsMapping mPushConstantsMapping;

	};

	class Pipeline;
	using PipelineRef = std::shared_ptr<Pipeline>;

	class Pipeline
	{

	public:

		struct Options
		{
			Options();
			
			Options& vertexInputBindingDescriptions(const std::vector<VkVertexInputBindingDescription>& tVertexInputBindingDescriptions) { mVertexInputBindingDescriptions = tVertexInputBindingDescriptions; return *this; }
			Options& vertexInputAttributeDescriptions(const std::vector<VkVertexInputAttributeDescription>& tVertexInputAttributeDescriptions) { mVertexInputAttributeDescriptions = tVertexInputAttributeDescriptions; return *this; }
			Options& vertexShader(const ShaderModuleRef &tShaderModule) { mVertexShader = tShaderModule; return *this; }
			Options& tessellationControlShader(const ShaderModuleRef &tShaderModule) { mTessellationControlShader = tShaderModule; return *this; }
			Options& tessellationEvaluationShader(const ShaderModuleRef &tShaderModule) { mTessellationEvaluationShader = tShaderModule; return *this; }
			Options& geometryShader(const ShaderModuleRef &tShaderModule) { mGeometryShader = tShaderModule; return *this; }
			Options& fragmentShader(const ShaderModuleRef &tShaderModule) { mFragmentShader = tShaderModule; return *this; }

			std::vector<VkVertexInputBindingDescription> mVertexInputBindingDescriptions;
			std::vector<VkVertexInputAttributeDescription> mVertexInputAttributeDescriptions;
			ShaderModuleRef mVertexShader;
			ShaderModuleRef mTessellationControlShader;
			ShaderModuleRef mTessellationEvaluationShader;
			ShaderModuleRef mGeometryShader;
			ShaderModuleRef mFragmentShader;
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
		VkPushConstantRange getPushConstantsMember(const std::string &tMemberName) const;

	private:

		VkPipelineShaderStageCreateInfo buildPipelineShaderStageCreateInfo(const ShaderModuleRef &tShaderModule, VkShaderStageFlagBits tShaderStageFlagBits);
		//! For each shader stage that is present during the pipeline creation process, add its push constant ranges to the global map.
		void buildPushConstantRanges();
		//! Given a shader module and shader stage, add all of the module's push constant ranges to the pipeline object's global map.
		void addPushConstantRangesToGlobalMap(const ShaderModuleRef &tShaderModule, VkShaderStageFlagBits tShaderStageFlagBits);

		VkPipeline mPipelineHandle;
		VkPipelineLayout mPipelineLayoutHandle;
		std::vector<VkPushConstantRange> mPushConstantRanges;

		DeviceRef mDevice;
		RenderPassRef mRenderPass;
		ShaderModuleRef mVertexShader;
		ShaderModuleRef mTessellationControlShader;
		ShaderModuleRef mTessellationEvaluationShader;
		ShaderModuleRef mGeometryShader;
		ShaderModuleRef mFragmentShader;

		std::map<std::string, VkPushConstantRange> mPushConstantRangesMapping;
	};

} // namespace vk