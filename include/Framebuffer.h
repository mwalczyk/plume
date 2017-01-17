#pragma once

#include <memory>
#include <vector>

#include "Platform.h"
#include "Device.h"
#include "RenderPass.h"

namespace vk
{

	class Framebuffer;
	using FramebufferRef = std::shared_ptr<Framebuffer>;

	class Framebuffer
	{

	public:

		struct Options
		{
			Options();

			Options& width(uint32_t tWidth) { mWidth = tWidth; return *this; }
			Options& height(uint32_t tHeight) { mHeight = tHeight; return *this; }

			uint32_t mWidth;
			uint32_t mHeight;
		};

		//! Factory method for returning a new FramebufferRef.
		static FramebufferRef create(const DeviceRef &tDevice, const RenderPassRef &tRenderPass, const std::vector<VkImageView> &tImageViews, const Options &tOptions = Options())
		{
			return std::make_shared<Framebuffer>(tDevice, tRenderPass, tImageViews, tOptions);
		}

		Framebuffer(const DeviceRef &tDevice, const RenderPassRef &tRenderPass, const std::vector<VkImageView> &tImageViews, const Options &tOptions = Options());
		~Framebuffer();

		inline VkFramebuffer getHandle() const { return mFramebufferHandle; };
		inline uint32_t getWidth() const { return mWidth; }
		inline uint32_t getHeight() const { return mHeight; }

		//! Retrieve the list of attachments associated with this framebuffer.
		inline const std::vector<VkImageView>& getImageViews() const { return mImageViews; }
		bool isCompatible(const RenderPassRef &tRenderPass);

	private:

		VkFramebuffer mFramebufferHandle;

		DeviceRef mDevice;
		RenderPassRef mRenderPass;
		std::vector<VkImageView> mImageViews;

		uint32_t mWidth;
		uint32_t mHeight;
	};

} // namespace vk