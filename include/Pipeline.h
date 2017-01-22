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
			
			Options& vertexInputBindingDescriptions(const std::vector<vk::VertexInputBindingDescription>& tVertexInputBindingDescriptions) { mVertexInputBindingDescriptions = tVertexInputBindingDescriptions; return *this; }
			Options& vertexInputAttributeDescriptions(const std::vector<vk::VertexInputAttributeDescription>& tVertexInputAttributeDescriptions) { mVertexInputAttributeDescriptions = tVertexInputAttributeDescriptions; return *this; }
			Options& viewport(const vk::Viewport &tViewport) { mViewport = tViewport; return *this; }
			Options& scissor(const vk::Rect2D &tScissor) { mScissor = tScissor; return *this; }
			Options& attachShaderStage(const ShaderModuleRef &tShaderModule, vk::ShaderStageFlagBits tShaderStageFlagBits) { mShaderStages.push_back({ tShaderModule, tShaderStageFlagBits }); return *this; }
			Options& polygonMode(vk::PolygonMode tPolygonMode) { mPolygonMode = tPolygonMode; return *this; }
			Options& lineWidth(float tLineWidth) { mLineWidth = tLineWidth; return *this; }
			Options& cullMode(vk::CullModeFlags tCullModeFlags) { mCullModeFlags = tCullModeFlags; return *this; }
			Options& primitiveRestart(bool tPrimitiveRestart) { mPrimitiveRestart = tPrimitiveRestart; return *this; }
			Options& primitiveTopology(vk::PrimitiveTopology tPrimitiveTopology) { mPrimitiveTopology = tPrimitiveTopology; return *this; }
			
			//! Configure per-attached framebuffer color blending, which determines how new fragments are composited with colors that are already in the framebuffer.
			Options& pipelineColorBlendAttachmentState(const vk::PipelineColorBlendAttachmentState &tPipelineColorBlendAttachmentState) { mPipelineColorBlendAttachmentState = tPipelineColorBlendAttachmentState; return *this; }
		
		private:

			std::vector<vk::VertexInputBindingDescription> mVertexInputBindingDescriptions;
			std::vector<vk::VertexInputAttributeDescription> mVertexInputAttributeDescriptions;
			vk::Viewport mViewport;
			vk::Rect2D mScissor;
			std::vector<std::pair<ShaderModuleRef, vk::ShaderStageFlagBits>> mShaderStages;
			vk::CullModeFlags mCullModeFlags;
			vk::PolygonMode mPolygonMode;
			float mLineWidth;
			bool mPrimitiveRestart;
			vk::PrimitiveTopology mPrimitiveTopology;
			vk::PipelineColorBlendAttachmentState mPipelineColorBlendAttachmentState;

			friend class Pipeline;
		};

		//! Factory method for returning a new PipelineRef.
		static PipelineRef create(const DeviceRef &tDevice, const RenderPassRef &tRenderPass, const Options &tOptions = Options()) 
		{ 
			return std::make_shared<Pipeline>(tDevice, tRenderPass, tOptions);
		}
		
		//! Helper function for constructing a vertex input binding description.
		static vk::VertexInputBindingDescription createVertexInputBindingDescription(uint32_t tBinding, uint32_t tStride, vk::VertexInputRate tVertexInputRate = vk::VertexInputRate::eVertex);

		//! Helper function for constructing a vertex input attribute description.
		static vk::VertexInputAttributeDescription createVertexInputAttributeDescription(uint32_t tBinding, vk::Format tFormat, uint32_t tLocation, uint32_t tOffset);

		//! Helper function for constructing a pipeline color blend attachment state that corresponds to standard alpha blending.
		static vk::PipelineColorBlendAttachmentState createAlphaBlendingAttachmentState();

		Pipeline(const DeviceRef &tDevice, const RenderPassRef &tRenderPass, const Options &tOptions = Options());
		~Pipeline();

		inline vk::Pipeline getHandle() const { return mPipelineHandle; }
		inline vk::PipelineLayout getPipelineLayoutHandle() const { return mPipelineLayoutHandle; }

		//! Returns a push constant range structure that holds information about the push constant with the given name.
		vk::PushConstantRange getPushConstantsMember(const std::string &tMemberName) const;

		//! Returns a descriptor set layout that holds information about the descriptor set with the given index.
		vk::DescriptorSetLayout getDescriptorSetLayout(uint32_t tSet) const;

		//! Given a descriptor set index, create and return a handle to a new descriptor pool whose size matches the combined 
		//! size of all of the descriptors in that set. If there is no descriptor set with the given index, return a null handle.
		vk::DescriptorPool createCompatibleDescriptorPool(uint32_t tSet, uint32_t tMaxSets = 1);

		friend std::ostream& operator<<(std::ostream &tStream, const PipelineRef &tPipeline);

	private:

		vk::PipelineShaderStageCreateInfo buildPipelineShaderStageCreateInfo(const ShaderModuleRef &tShaderModule, vk::ShaderStageFlagBits tShaderStageFlagBits);

		//! Given a shader module and shader stage, add all of the module's push constants to the pipeline object's global map.
		void addPushConstantsToGlobalMap(const ShaderModuleRef &tShaderModule, vk::ShaderStageFlagBits tShaderStageFlagBits);

		//! Given a shader module and shader stage, add all of the module's descriptors to the pipeline object's global map.
		void addDescriptorsToGlobalMap(const ShaderModuleRef &tShaderModule, vk::ShaderStageFlagBits tShaderStageFlagBits);

		//! Generate all of the descriptor set layout handles.		
		void buildDescriptorSetLayouts();

		DeviceRef mDevice;
		RenderPassRef mRenderPass;
		vk::Pipeline mPipelineHandle;
		vk::PipelineLayout mPipelineLayoutHandle;
		std::map<std::string, vk::PushConstantRange> mPushConstantsMapping;
		std::map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>> mDescriptorsMapping;
		std::map<uint32_t, vk::DescriptorSetLayout> mDescriptorSetLayoutsMapping;
	};

} // namespace vksp