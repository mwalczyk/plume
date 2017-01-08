#include "RenderPass.h"

namespace vk
{

	RenderPass::AttachmentInfo RenderPass::createColorAttachmentDescription(const VkFormat &tFormat, uint32_t tAttachment)
	{	
		AttachmentInfo attachmentInfo = {};

		// Set up the attachment description
		attachmentInfo.mAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachmentInfo.mAttachmentDescription.format = tFormat;
		attachmentInfo.mAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentInfo.mAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentInfo.mAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentInfo.mAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentInfo.mAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentInfo.mAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		
		// Set up the attachment reference
		attachmentInfo.mAttachmentReference.attachment = tAttachment;
		attachmentInfo.mAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		return attachmentInfo;
	}

	RenderPass::Options::Options()
	{

	}

	RenderPass::RenderPass(const DeviceRef &tDevice, const Options &tOptions) :
		mDevice(tDevice)
	{
		auto attachmentInfo = createColorAttachmentDescription(VK_FORMAT_B8G8R8A8_UNORM, 0);

		// A subpass description describes the subset of attachments that is involved in the execution of each subpass.
		// Each subpass can read from some attachments as input attachments, write to some as color attachments or 
		// depth / stencil attachments, and perform multisample resolve operations to resolve attachments.
		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;	// Currently, only graphics subpasses are supported by Vulkan.
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &attachmentInfo.mAttachmentReference;

		// A subpass dependency describes the execution and memory dependencies between subpasses.
		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		// Create a render pass with the information above.
		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.dependencyCount = 1;
		renderPassCreateInfo.pAttachments = &attachmentInfo.mAttachmentDescription;
		renderPassCreateInfo.pDependencies = &subpassDependency;
		renderPassCreateInfo.pSubpasses = &subpassDescription;
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.subpassCount = 1;

		auto result = vkCreateRenderPass(mDevice->getHandle(), &renderPassCreateInfo, nullptr, &mRenderPassHandle);
		assert(result == VK_SUCCESS);

		std::cout << "Successfully created render pass\n";
	}

	RenderPass::~RenderPass()
	{
		vkDestroyRenderPass(mDevice->getHandle(), mRenderPassHandle, nullptr);
	}

} // namespace vk
