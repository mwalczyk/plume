#pragma once

#include <memory>
#include <vector>

#include "Platform.h"
#include "Device.h"
#include "Semaphore.h"

namespace vk
{

	class Swapchain;
	using SwapchainRef = std::shared_ptr<Swapchain>;

	class Swapchain
	{
	public:

		class Options
		{
		public:

			Options();

			//! Set the preferred presentation mode (defaults is VK_PRESENT_MODE_MAILBOX_KHR).
			Options& presentMode(VkPresentModeKHR tPresentMode) { mPresentMode = tPresentMode; return *this; }

		private:

			VkPresentModeKHR mPresentMode;

			friend class Swapchain;
		};

		//! Factory method for returning a new SwapchainRef.
		static SwapchainRef create(const DeviceRef &tDevice, const SurfaceRef &tSurface, uint32_t tWidth, uint32_t tHeight, const Options &tOptions = Options())
		{
			return std::make_shared<Swapchain>(tDevice, tSurface, tWidth, tHeight, tOptions);
		}

		Swapchain(const DeviceRef &tDevice, const SurfaceRef &tSurface, uint32_t tWidth, uint32_t tHeight, const Options &tOptions = Options());
		~Swapchain();

		inline VkSwapchainKHR getHandle() const { return mSwapchainHandle; };
		inline const std::vector<VkImage>& getImageHandles() const { return mImageHandles; }
		inline const std::vector<VkImageView>& getImageViewHandles() const { return mImageViewHandles; }
		uint32_t acquireNextSwapchainImage(const SemaphoreRef &tSemaphore, uint32_t tNanosecondsTimeout = std::numeric_limits<uint64_t>::max());

	private:

		VkSurfaceFormatKHR selectSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &tSurfaceFormats) const;
		VkPresentModeKHR selectSwapchainPresentMode(const std::vector<VkPresentModeKHR> &tPresentModes) const;
		VkExtent2D selectSwapchainExtent(const VkSurfaceCapabilitiesKHR &tSurfaceCapabilities) const;
		void createImageViews();

		DeviceRef mDevice;
		SurfaceRef mSurface;

		VkSwapchainKHR mSwapchainHandle;
		VkFormat mSwapchainImageFormat;
		VkExtent2D mSwapchainImageExtent;
		std::vector<VkImage> mImageHandles;
		std::vector<VkImageView> mImageViewHandles;

		uint32_t mWidth;
		uint32_t mHeight;
	};

} // namespace vk