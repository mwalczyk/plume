#pragma once

#include <memory>
#include <vector>
#include <map>
#include <iterator>
#include <string>
#include <ostream>

#include "Platform.h"
#include "Device.h"
#include "RenderPass.h"
#include "ShaderModule.h"

namespace vksp
{

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
			Options& polygonMode(VkPolygonMode tPolygonMode) { mPolygonMode = tPolygonMode; return *this; }
			Options& lineWidth(float tLineWidth) { mLineWidth = tLineWidth; return *this; }
			Options& cullMode(VkCullModeFlags tCullModeFlags) { mCullModeFlags = tCullModeFlags; return *this; }
			Options& primitiveRestart(bool tPrimitiveRestart) { mPrimitiveRestart = tPrimitiveRestart; return *this; }
			Options& primitiveTopology(VkPrimitiveTopology tPrimitiveTopology) { mPrimitiveTopology = tPrimitiveTopology; return *this; }
			
			//! Configure per-attached framebuffer color blending, which determines how new fragments are composited with colors that are already in the framebuffer.
			Options& pipelineColorBlendAttachmentState(const VkPipelineColorBlendAttachmentState &tPipelineColorBlendAttachmentState) { mPipelineColorBlendAttachmentState = tPipelineColorBlendAttachmentState; return *this; }
		
		private:

			std::vector<VkVertexInputBindingDescription> mVertexInputBindingDescriptions;
			std::vector<VkVertexInputAttributeDescription> mVertexInputAttributeDescriptions;
			VkViewport mViewport;
			VkRect2D mScissor;
			std::vector<std::pair<ShaderModuleRef, VkShaderStageFlagBits>> mShaderStages;
			VkCullModeFlags mCullModeFlags;
			VkPolygonMode mPolygonMode;
			float mLineWidth;
			bool mPrimitiveRestart;
			VkPrimitiveTopology mPrimitiveTopology;
			VkPipelineColorBlendAttachmentState mPipelineColorBlendAttachmentState;

			friend class Pipeline;
		};

		//! Factory method for returning a new PipelineRef.
		static PipelineRef create(const DeviceRef &tDevice, const RenderPassRef &tRenderPass, const Options &tOptions = Options()) 
		{ 
			return std::make_shared<Pipeline>(tDevice, tRenderPass, tOptions);
		}
		
		//! Helper function for constructing a vertex input binding description.
		static VkVertexInputBindingDescription createVertexInputBindingDescription(uint32_t tBinding, uint32_t tStride, VkVertexInputRate tVertexInputRate = VK_VERTEX_INPUT_RATE_VERTEX);

		//! Helper function for constructing a vertex input attribute description.
		static VkVertexInputAttributeDescription createVertexInputAttributeDescription(uint32_t tBinding, VkFormat tFormat, uint32_t tLocation, uint32_t tOffset);

		//! Helper function for constructing a pipeline color blend attachment state that corresponds to standard alpha blending.
		static VkPipelineColorBlendAttachmentState createAlphaBlendingAttachmentState();

		Pipeline(const DeviceRef &tDevice, const RenderPassRef &tRenderPass, const Options &tOptions = Options());
		~Pipeline();

		inline VkPipeline getHandle() const { return mPipelineHandle; }
		inline VkPipelineLayout getPipelineLayoutHandle() const { return mPipelineLayoutHandle; }

		//! Returns a push constant range structure that holds information about the push constant with the given name.
		VkPushConstantRange getPushConstantsMember(const std::string &tMemberName) const;

		//! Returns a descriptor set layout that holds information about the descriptor set with the given index.
		VkDescriptorSetLayout getDescriptorSetLayout(uint32_t tSet) const;

		//! Given a descriptor set index, create and return a handle to a new descriptor pool whose size matches the combined 
		//! size of all of the descriptors in that set. If there is no descriptor set with the given index, return a null handle.
		VkDescriptorPool createCompatibleDescriptorPool(uint32_t tSet, uint32_t tMaxSets = 1);

		friend std::ostream& operator<<(std::ostream &tStream, const PipelineRef &tPipeline);

	private:

		VkPipelineShaderStageCreateInfo buildPipelineShaderStageCreateInfo(const ShaderModuleRef &tShaderModule, VkShaderStageFlagBits tShaderStageFlagBits);

		//! Given a shader module and shader stage, add all of the module's push constants to the pipeline object's global map.
		void addPushConstantsToGlobalMap(const ShaderModuleRef &tShaderModule, VkShaderStageFlagBits tShaderStageFlagBits);

		//! Given a shader module and shader stage, add all of the module's descriptors to the pipeline object's global map.
		void addDescriptorsToGlobalMap(const ShaderModuleRef &tShaderModule, VkShaderStageFlagBits tShaderStageFlagBits);

		//! Generate all of the descriptor set layout handles.		
		void buildDescriptorSetLayouts();

		VkPipeline mPipelineHandle;
		VkPipelineLayout mPipelineLayoutHandle;
		std::map<std::string, VkPushConstantRange> mPushConstantsMapping;
		std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> mDescriptorsMapping;
		std::map<uint32_t, VkDescriptorSetLayout> mDescriptorSetLayoutsMapping;

		DeviceRef mDevice;
		RenderPassRef mRenderPass;
	};

} // namespace vksp