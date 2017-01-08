#pragma once

#include <memory>
#include <vector>

#include "Platform.h"
#include "Device.h"

namespace vk
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

		struct AttachmentInfo
		{
			//! An attachment description describes the properties of an attachment including its format, sample count, and
			//! how its contents ae treated at the beginning and end of each render pass instance.
			VkAttachmentDescription mAttachmentDescription;

			//! An attachment reference simply stores an index and a reference to an attachment description. It is used for
			//! constructing a subpass description.
			VkAttachmentReference mAttachmentReference;
		};

		struct Options
		{
			Options();

			Options& attachmentDescriptions(const std::vector<VkAttachmentDescription> &tAttachmentDescriptions) { mAttachmentDescriptions = tAttachmentDescriptions; return *this; }
			Options& subpassDependencies(const std::vector<VkSubpassDependency> &tSubpassDependencies) { mSubpassDependencies = tSubpassDependencies; return *this; }

			std::vector<VkAttachmentDescription> mAttachmentDescriptions;
			std::vector<VkSubpassDependency> mSubpassDependencies;
		};

		//! Factory method for returning a new RenderPassRef.
		static RenderPassRef create(const DeviceRef &tDevice, const Options &tOptions = Options())
		{
			return std::make_shared<RenderPass>(tDevice, tOptions);
		}

		static AttachmentInfo createColorAttachmentDescription(const VkFormat &tFormat, uint32_t tAttachment);
		static AttachmentInfo createDepthStencilAttachmentDescription(const VkFormat &tFormat, uint32_t tAttachment);

		RenderPass(const DeviceRef &tDevice, const Options &tOptions = Options());
		~RenderPass();

		inline VkRenderPass getHandle() const { return mRenderPassHandle; }

	private:

		VkRenderPass mRenderPassHandle;

		DeviceRef mDevice;

		std::vector<VkAttachmentDescription> mAttachmentDescriptions;
		std::vector<VkSubpassDependency> mSubpassDependencies;

	};

} // namespace vk