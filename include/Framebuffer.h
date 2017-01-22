#pragma once

#include <memory>
#include <vector>

#include "Platform.h"
#include "Device.h"
#include "RenderPass.h"

namespace vksp
{

	class Framebuffer;
	using FramebufferRef = std::shared_ptr<Framebuffer>;

	class Framebuffer
	{
	public:

		class Options
		{
		public:

			Options();

		private:

			friend class Framebuffer;
		};

		//! Factory method for returning a new FramebufferRef.
		static FramebufferRef create(const DeviceRef &tDevice, const RenderPassRef &tRenderPass, const std::vector<VkImageView> &tImageViews, uint32_t tWidth, uint32_t tHeight, const Options &tOptions = Options())
		{
			return std::make_shared<Framebuffer>(tDevice, tRenderPass, tImageViews, tWidth, tHeight, tOptions);
		}

		Framebuffer(const DeviceRef &tDevice, const RenderPassRef &tRenderPass, const std::vector<VkImageView> &tImageViews, uint32_t tWidth, uint32_t tHeight, const Options &tOptions = Options());
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

} // namespace vksp