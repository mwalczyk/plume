#include "Framebuffer.h"

namespace vk
{

	Framebuffer::Options::Options()
	{
		mWidth = 640;
		mHeight = 480;
	}

	Framebuffer::Framebuffer(const DeviceRef &tDevice, const RenderPassRef &tRenderPass, const std::vector<VkImageView> &tImageViews, const Options &tOptions) :
		mDevice(tDevice),
		mRenderPass(tRenderPass),
		mImageViews(tImageViews),
		mWidth(tOptions.mWidth),
		mHeight(tOptions.mHeight)
	{
		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(mImageViews.size());
		framebufferCreateInfo.height = mHeight;
		framebufferCreateInfo.layers = 1;
		framebufferCreateInfo.pAttachments = mImageViews.data();
		framebufferCreateInfo.renderPass = mRenderPass->getHandle();		// The render pass that this framebuffer needs to be compatible with.
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.width = mWidth;

		auto result = vkCreateFramebuffer(mDevice->getHandle(), &framebufferCreateInfo, nullptr, &mFramebufferHandle);
		assert(result == VK_SUCCESS);

		std::cout << "Successfully created framebuffer with " << mImageViews.size() << " attachments\n";
	}

	Framebuffer::~Framebuffer()
	{
		vkDestroyFramebuffer(mDevice->getHandle(), mFramebufferHandle, nullptr);
	}

} // namespace vk