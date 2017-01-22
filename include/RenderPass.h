#pragma once

#include <memory>
#include <vector>

#include "Platform.h"
#include "Device.h"

namespace vksp
{

	class RenderPass;
	using RenderPassRef = std::shared_ptr<RenderPass>;

	//! In Vulkan, a render pass represents a collection of attachments, subpasses, and dependencies between
	//! the subpasses, and describes how the attachments are used over the course of the subpasses. A subpass
	//! represents a phase of rendering that reads and writes a subset of the attachments in a render pass.
	//! Rendering commands are recorded into a particular subpass of a render pass. The specific image views
	//! that will be used for the attachments are specified by framebuffer objects. Framebuffers are created
	//! with respect to a specific render pass that the framebuffer is compatible with.
	class RenderPass
	{
	public:

		class Options
		{
		public:

			Options();

			//! An attachment description describes the properties of an attachment including its format, sample count, and
			//! how its contents ae treated at the beginning and end of each render pass instance.
			Options& attachmentDescriptions(const std::vector<VkAttachmentDescription> &tAttachmentDescriptions) { mAttachmentDescriptions = tAttachmentDescriptions; return *this; }
			
			//! An attachment reference simply stores an index and a reference to an attachment description. It is used for
			//! constructing a subpass description.
			Options& attachmentReferences(const std::vector<VkAttachmentReference> &tAttachmentReferences) { mAttachmentReferences = tAttachmentReferences; return *this; }
			
			//! A subpass description describes the subset of attachments that is involved in the execution of each subpass.
			//! Each subpass can read from some attachments as input attachments, write to some as color attachments or 
			//! depth / stencil attachments, and perform multisample resolve operations to resolve attachments.
			Options& subpassDescriptions(const std::vector<VkSubpassDescription> &tSubpassDescriptions) { mSubpassDescriptions = tSubpassDescriptions; return *this; }

			//! A subpass dependency describes the execution and memory dependencies between subpasses.
			Options& subpassDependencies(const std::vector<VkSubpassDependency> &tSubpassDependencies) { mSubpassDependencies = tSubpassDependencies; return *this; }

		private:

			std::vector<VkAttachmentDescription> mAttachmentDescriptions;
			std::vector<VkAttachmentReference> mAttachmentReferences;
			std::vector<VkSubpassDescription> mSubpassDescriptions;
			std::vector<VkSubpassDependency> mSubpassDependencies;

			friend class RenderPass;
		};

		//! Factory method for returning a new RenderPassRef.
		static RenderPassRef create(const DeviceRef &tDevice, const Options &tOptions = Options())
		{
			return std::make_shared<RenderPass>(tDevice, tOptions);
		}

		static std::pair<VkAttachmentDescription, VkAttachmentReference> createColorAttachment(const VkFormat &tFormat, uint32_t tAttachment);
		static std::pair<VkAttachmentDescription, VkAttachmentReference> createDepthStencilAttachment(const VkFormat &tFormat, uint32_t tAttachment);
		static VkSubpassDescription createSubpassDescription(const std::vector<VkAttachmentReference> &tAttachmentReferences, VkPipelineBindPoint tPipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);
		static VkSubpassDependency createDefaultSubpassDependency();

		RenderPass(const DeviceRef &tDevice, const Options &tOptions = Options());
		~RenderPass();

		inline VkRenderPass getHandle() const { return mRenderPassHandle; }

	private:

		VkRenderPass mRenderPassHandle;

		DeviceRef mDevice;

		std::vector<VkAttachmentDescription> mAttachmentDescriptions;
		std::vector<VkAttachmentReference> mAttachmentReferences;
		std::vector<VkSubpassDescription> mSubpassDescriptions;
		std::vector<VkSubpassDependency> mSubpassDependencies;
	};

} // namespace vksp