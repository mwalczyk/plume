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

		//! A struct representing a memmber within a push constants block inside of a GLSL shader. For example:
		//! layout (std430, push_constant) uniform PushConstants
		//! {
		//!		float time;		<--- this
		//! } constants;
		struct PushConstant
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

		//! A struct representing a descriptor inside of a GLSL shader. For example:
		//! layout (set = 0, binding = 1) uniform UniformBufferObject	<--- this
		//! {
		//!		mat4 model;
		//!		mat4 view;
		//!		mat4 projection
		//! } ubo;
		struct Descriptor
		{
			uint32_t layoutSet;
			uint32_t layoutBinding;
			uint32_t descriptorCount;
			VkDescriptorType descriptorType;
			std::string name;
		};
		
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

		//! Retrieve a list of low-level details about the push constants contained within this GLSL shader.
		inline const std::vector<PushConstant> getPushConstants() const { return mPushConstants; }

		//! Retrieve a list of low-level details about the descriptors contained within this GLSL shader.
		inline const std::vector<Descriptor>& getDescriptors() const { return mDescriptors; }

	private:

		void performReflection();

		DeviceRef mDevice;

		VkShaderModule mShaderModuleHandle;

		std::vector<uint32_t> mShaderCode;
		std::vector<std::string> mEntryPoints;
		std::vector<StageInput> mStageInputs;
		std::vector<PushConstant> mPushConstants;
		std::vector<Descriptor> mDescriptors;

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
			Options& attachShaderStage(const ShaderModuleRef &tShaderModule, VkShaderStageFlagBits tShaderStageFlagBits) { mShaderStages.push_back({ tShaderModule, tShaderStageFlagBits }); return *this; }
			Options& primitiveRestart(bool tPrimitiveRestart) { mPrimitiveRestart = tPrimitiveRestart; return *this; }
			Options& primitiveTopology(VkPrimitiveTopology tPrimitiveTopology) { mPrimitiveTopology = tPrimitiveTopology; return *this; }

		private:

			std::vector<VkVertexInputBindingDescription> mVertexInputBindingDescriptions;
			std::vector<VkVertexInputAttributeDescription> mVertexInputAttributeDescriptions;
			VkViewport mViewport;
			VkRect2D mScissor;
			std::vector<std::pair<ShaderModuleRef, VkShaderStageFlagBits>> mShaderStages;
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

		friend std::ostream& operator<<(std::ostream &tStream, const PipelineRef &tPipeline);

	private:

		VkPipelineShaderStageCreateInfo buildPipelineShaderStageCreateInfo(const ShaderModuleRef &tShaderModule, VkShaderStageFlagBits tShaderStageFlagBits);

		//! Given a shader module and shader stage, add all of the module's push constants to the pipeline object's global map.
		void addPushConstantsToGlobalMap(const ShaderModuleRef &tShaderModule, VkShaderStageFlagBits tShaderStageFlagBits);

		//! Given a shader module and shader stage, add all of the module's descriptors to the pipeline object's global map.
		void addDescriptorsToGlobalMap(const ShaderModuleRef &tShaderModule, VkShaderStageFlagBits tShaderStageFlagBits);

		VkPipeline mPipelineHandle;
		VkPipelineLayout mPipelineLayoutHandle;
		std::vector<VkDescriptorSetLayout> mDescriptorSetLayoutHandles;

		DeviceRef mDevice;
		RenderPassRef mRenderPass;

		std::map<std::string, VkPushConstantRange> mPushConstantsMapping;
		std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> mDescriptorsMapping;

	};

} // namespace vk