#include "RenderPass.h"

namespace graphics
{

	std::pair<vk::AttachmentDescription, vk::AttachmentReference> RenderPass::createColorAttachment(const vk::Format &tFormat, uint32_t tAttachment)
	{
		// Set up the attachment description
		vk::AttachmentDescription attachmentDescription;
		attachmentDescription.finalLayout = vk::ImageLayout::ePresentSrcKHR;
		attachmentDescription.format = tFormat;
		attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
		attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
		attachmentDescription.samples = vk::SampleCountFlagBits::e1;
		attachmentDescription.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;

		// Set up the attachment reference
		vk::AttachmentReference attachmentReference;
		attachmentReference.attachment = tAttachment;
		attachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal; 

		return { attachmentDescription, attachmentReference };
	}

	std::pair<vk::AttachmentDescription, vk::AttachmentReference> RenderPass::createDepthStencilAttachment(const vk::Format &tFormat, uint32_t tAttachment)
	{
		if (!ImageBase::isDepthFormat(tFormat))
		{
			throw std::runtime_error("Attempting to create a depth stencil attachment with an invalid image format");
		}

		// Set up the attachment description
		vk::AttachmentDescription attachmentDescription;
		attachmentDescription.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		attachmentDescription.format = tFormat;
		attachmentDescription.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		attachmentDescription.loadOp = vk::AttachmentLoadOp::eClear;
		attachmentDescription.samples = vk::SampleCountFlagBits::e1;
		attachmentDescription.stencilLoadOp = ImageBase::isStencilFormat(tFormat) ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare;
		attachmentDescription.stencilStoreOp = ImageBase::isStencilFormat(tFormat) ? vk::AttachmentStoreOp::eStore : vk::AttachmentStoreOp::eDontCare;
		attachmentDescription.storeOp = vk::AttachmentStoreOp::eDontCare;

		// Set up the attachment reference
		vk::AttachmentReference attachmentReference;
		attachmentReference.attachment = tAttachment;
		attachmentReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		return { attachmentDescription, attachmentReference };
	}

	vk::SubpassDescription RenderPass::createSubpassDescription(const std::vector<vk::AttachmentReference> &tColorAttachmentReferences, const vk::AttachmentReference &tDepthStencilAttachmentReference)
	{
		vk::SubpassDescription subpassDescription;
		subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;	// Currently, only graphics subpasses are supported by Vulkan.
		subpassDescription.colorAttachmentCount = static_cast<uint32_t>(tColorAttachmentReferences.size());
		subpassDescription.pColorAttachments = tColorAttachmentReferences.data();
		subpassDescription.pDepthStencilAttachment = &tDepthStencilAttachmentReference;

		return subpassDescription;
	}

	vk::SubpassDependency RenderPass::createDefaultSubpassDependency()
	{
		vk::SubpassDependency subpassDependency;
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput; 
		subpassDependency.srcAccessMask = {};
		subpassDependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		subpassDependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

		return subpassDependency;
	}

	RenderPass::Options::Options()
	{
		auto colorAttachment = createColorAttachment(vk::Format::eB8G8R8A8Unorm, 0);
		auto depthAttachment = createDepthStencilAttachment(vk::Format::eD32SfloatS8Uint, 1);

		// Aggregate all attachment references and descriptions.
		mAttachmentDescriptions = { colorAttachment.first, depthAttachment.first };
		mAttachmentReferences = { colorAttachment.second, depthAttachment.second };

		vk::SubpassDescription subpassDescription;
		subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;	
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &mAttachmentReferences[0];
		subpassDescription.pDepthStencilAttachment = &mAttachmentReferences[1];
		subpassDescription.preserveAttachmentCount = 0;
		subpassDescription.pPreserveAttachments = nullptr;
		subpassDescription.pResolveAttachments = nullptr;

		mSubpassDescriptions.emplace_back(subpassDescription);
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
		vk::RenderPassCreateInfo renderPassCreateInfo;
		renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(mAttachmentDescriptions.size());
		renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(mSubpassDependencies.size());
		renderPassCreateInfo.pAttachments = mAttachmentDescriptions.data();
		renderPassCreateInfo.pDependencies = mSubpassDependencies.data();
		renderPassCreateInfo.pSubpasses = mSubpassDescriptions.data();
		renderPassCreateInfo.subpassCount = static_cast<uint32_t>(mSubpassDescriptions.size());

		mRenderPassHandle = mDevice->getHandle().createRenderPass(renderPassCreateInfo);
	}

	RenderPass::~RenderPass()
	{
		mDevice->getHandle().destroyRenderPass(mRenderPassHandle);
	}

} // namespace graphics
