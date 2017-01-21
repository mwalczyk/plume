#include "RenderPass.h"

namespace vk
{

	std::pair<VkAttachmentDescription, VkAttachmentReference> RenderPass::createColorAttachment(const VkFormat &tFormat, uint32_t tAttachment)
	{
		// Set up the attachment description
		VkAttachmentDescription attachmentDescription = {};
		attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachmentDescription.format = tFormat;
		attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		// Set up the attachment reference
		VkAttachmentReference attachmentReference = {};
		attachmentReference.attachment = tAttachment;
		attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		return { attachmentDescription, attachmentReference };
	}

	VkSubpassDescription RenderPass::createSubpassDescription(const std::vector<VkAttachmentReference> &tAttachmentReferences, VkPipelineBindPoint tPipelineBindPoint)
	{
		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = tPipelineBindPoint;		// Currently, only graphics subpasses are supported by Vulkan.
		subpassDescription.colorAttachmentCount = static_cast<uint32_t>(tAttachmentReferences.size());
		subpassDescription.pColorAttachments = tAttachmentReferences.data();

		return subpassDescription;
	}

	VkSubpassDependency RenderPass::createDefaultSubpassDependency()
	{
		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		return subpassDependency;
	}

	RenderPass::Options::Options()
	{
		auto colorAttachment = createColorAttachment(VK_FORMAT_B8G8R8A8_UNORM, 0);

		mAttachmentDescriptions.emplace_back(colorAttachment.first);
		mAttachmentReferences.emplace_back(colorAttachment.second);
		mSubpassDescriptions.emplace_back(createSubpassDescription(mAttachmentReferences));
		mSubpassDependencies.emplace_back(createDefaultSubpassDependency());
	}

	RenderPass::RenderPass(const DeviceRef &tDevice, const Options &tOptions) :
		mDevice(tDevice),
		mAttachmentDescriptions(tOptions.mAttachmentDescriptions),
		mAttachmentReferences(tOptions.mAttachmentReferences),
		mSubpassDescriptions(tOptions.mSubpassDescriptions),
		mSubpassDependencies(tOptions.mSubpassDependencies)
	{
		// Create a render pass with the information above.
		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(mAttachmentDescriptions.size());
		renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(mSubpassDependencies.size());
		renderPassCreateInfo.pAttachments = mAttachmentDescriptions.data();
		renderPassCreateInfo.pDependencies = mSubpassDependencies.data();
		renderPassCreateInfo.pSubpasses = mSubpassDescriptions.data();
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.subpassCount = static_cast<uint32_t>(mSubpassDescriptions.size());

		auto result = vkCreateRenderPass(mDevice->getHandle(), &renderPassCreateInfo, nullptr, &mRenderPassHandle);
		assert(result == VK_SUCCESS);
	}

	RenderPass::~RenderPass()
	{
		vkDestroyRenderPass(mDevice->getHandle(), mRenderPassHandle, nullptr);
	}

} // namespace vk
