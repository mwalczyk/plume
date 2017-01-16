#pragma once

#include <memory>
#include <vector>
#include <map>
#include <iterator>
#include <fstream>
#include <string>
#include <utility>

#include "spirv_glsl.hpp"

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

		//! A struct representing a push constants block inside of a GLSL shader. For example:
		//! layout (std430, push_constant) uniform PushConstants	<--- this
		//! {
		//!		float time;
		//! } constants;
		struct PushConstantsBlock
		{
			uint32_t layoutLocation;
			uint32_t totalSize;
			std::string name;
		};

		//! A struct representing a memmber within a push constants block inside of a GLSL shader. For example:
		//! layout (std430, push_constant) uniform PushConstants
		//! {
		//!		float time;		<--- this
		//! } constants;
		struct PushConstantsMember
		{
			uint32_t index;
			uint32_t size;
			uint32_t offset;
			std::string name;
		};

		//! A struct representing an input to a shader stage. For example:
		//! layout (location = 0) in vec3 inPosition;
		struct StageInput
		{
			uint32_t layoutLocation;
			uint32_t size;
			std::string name;
		};

		//! A functor class that is used for constructing a mapping between PushConstantsBlock structures and PushConstantsMember structures.
		struct PushConstantsBlockOrdering
		{
			bool operator() (const PushConstantsBlock& lhs, const PushConstantsBlock& rhs) const
			{
				return lhs.layoutLocation < rhs.layoutLocation;
			}
		};

		//! A mapping between PushConstantsBlock structures and PushConstantsMember structures.
		using PushConstantsBlocksMapping = std::map<PushConstantsBlock, std::vector<PushConstantsMember>, PushConstantsBlockOrdering>;

		//! Factory method for returning a new ShaderModuleRef.
		static ShaderModuleRef create(const DeviceRef &tDevice, const std::string &tFilePath)
		{
			return std::make_shared<ShaderModule>(tDevice, tFilePath);
		}

		ShaderModule(const DeviceRef &tDevice, const std::string &tFilePath);
		~ShaderModule();

		inline VkShaderModule getHandle() const { return mShaderModuleHandle; }

		//! Retrieve the binary SPIR-V shader code that is held by this shader.
		inline const std::vector<uint32_t>& getShaderCode() const { return mShaderCode; }

		//! Retrieve a list of available entry points within this GLSL shader (usually "main").
		inline const std::vector<std::string>& getEntryPoints() const { return mEntryPoints; }

		//! Retrieve a map of low-level details about the push constant blocks contained within this GLSL shader.
		inline const PushConstantsBlocksMapping& getPushConstantsBlocksMapping() const { return mPushConstantsBlocksMapping; }

	private:

		void performReflection();

		DeviceRef mDevice;

		VkShaderModule mShaderModuleHandle;

		std::vector<uint32_t> mShaderCode;
		std::vector<std::string> mEntryPoints;
		PushConstantsBlocksMapping mPushConstantsBlocksMapping;
		std::vector<StageInput> mStageInputs;
	};

	class Pipeline;
	using PipelineRef = std::shared_ptr<Pipeline>;

	class Pipeline
	{

	public:

		class Options
		{

		public:

			Options();
			
			Options& vertexInputBindingDescriptions(const std::vector<VkVertexInputBindingDescription>& tVertexInputBindingDescriptions) { mVertexInputBindingDescriptions = tVertexInputBindingDescriptions; return *this; }
			Options& vertexInputAttributeDescriptions(const std::vector<VkVertexInputAttributeDescription>& tVertexInputAttributeDescriptions) { mVertexInputAttributeDescriptions = tVertexInputAttributeDescriptions; return *this; }
			Options& viewport(const VkViewport &tViewport) { mViewport = tViewport; return *this; }
			Options& scissor(const VkRect2D &tScissor) { mScissor = tScissor; return *this; }
			Options& vertexShader(const ShaderModuleRef &tShaderModule) { mVertexShader = tShaderModule; return *this; }
			Options& tessellationControlShader(const ShaderModuleRef &tShaderModule) { mTessellationControlShader = tShaderModule; return *this; }
			Options& tessellationEvaluationShader(const ShaderModuleRef &tShaderModule) { mTessellationEvaluationShader = tShaderModule; return *this; }
			Options& geometryShader(const ShaderModuleRef &tShaderModule) { mGeometryShader = tShaderModule; return *this; }
			Options& fragmentShader(const ShaderModuleRef &tShaderModule) { mFragmentShader = tShaderModule; return *this; }
			Options& primitiveRestart(bool tPrimitiveRestart) { mPrimitiveRestart = tPrimitiveRestart; return *this; }
			Options& primitiveTopology(VkPrimitiveTopology tPrimitiveTopology) { mPrimitiveTopology = tPrimitiveTopology; return *this; }

		private:

			std::vector<VkVertexInputBindingDescription> mVertexInputBindingDescriptions;
			std::vector<VkVertexInputAttributeDescription> mVertexInputAttributeDescriptions;
			VkViewport mViewport;
			VkRect2D mScissor;
			ShaderModuleRef mVertexShader;
			ShaderModuleRef mTessellationControlShader;
			ShaderModuleRef mTessellationEvaluationShader;
			ShaderModuleRef mGeometryShader;
			ShaderModuleRef mFragmentShader;
			bool mPrimitiveRestart;
			VkPrimitiveTopology mPrimitiveTopology;

			friend class Pipeline;

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

		//! Given a shader module and shader stage, add all of the module's push constant ranges to the pipeline object's global map.
		void addPushConstantRangesToGlobalMap(const ShaderModuleRef &tShaderModule, VkShaderStageFlagBits tShaderStageFlagBits);

		VkPipeline mPipelineHandle;
		VkPipelineLayout mPipelineLayoutHandle;

		DeviceRef mDevice;
		RenderPassRef mRenderPass;

		std::map<std::string, VkPushConstantRange> mPushConstantRangesMapping;

	};

} // namespace vk