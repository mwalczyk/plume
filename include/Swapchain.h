#pragma once

#include <memory>
#include <vector>

#include "Platform.h"
#include "Device.h"
#include "Semaphore.h"

namespace graphics
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
			
			//! Set the preferred presentation mode (defaults is vk::PresentModeKHR::eMailbox).
			Options& presentMode(vk::PresentModeKHR tPresentMode) { mPresentMode = tPresentMode; return *this; }

		private:

			vk::PresentModeKHR mPresentMode;

			friend class Swapchain;
		};

		//! Factory method for returning a new SwapchainRef.
		static SwapchainRef create(const DeviceRef &tDevice, const SurfaceRef &tSurface, uint32_t tWidth, uint32_t tHeight, const Options &tOptions = Options())
		{
			return std::make_shared<Swapchain>(tDevice, tSurface, tWidth, tHeight, tOptions);
		}

		Swapchain(const DeviceRef &tDevice, const SurfaceRef &tSurface, uint32_t tWidth, uint32_t tHeight, const Options &tOptions = Options());
		~Swapchain();

		inline vk::SwapchainKHR getHandle() const { return mSwapchainHandle; };
		inline const std::vector<vk::Image>& getImageHandles() const { return mImageHandles; }
		inline const std::vector<vk::ImageView>& getImageViewHandles() const { return mImageViewHandles; }
		uint32_t acquireNextSwapchainImage(const SemaphoreRef &tSemaphore, uint32_t tNanosecondsTimeout = std::numeric_limits<uint64_t>::max());

	private:

		vk::SurfaceFormatKHR selectSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &tSurfaceFormats) const;
		vk::PresentModeKHR selectSwapchainPresentMode(const std::vector<vk::PresentModeKHR> &tPresentModes) const;
		vk::Extent2D selectSwapchainExtent(const vk::SurfaceCapabilitiesKHR &tSurfaceCapabilities) const;
		void createImageViews();

		DeviceRef mDevice;
		SurfaceRef mSurface;
		vk::SwapchainKHR mSwapchainHandle;
		vk::Format mSwapchainImageFormat;
		vk::Extent2D mSwapchainImageExtent;
		std::vector<vk::Image> mImageHandles;
		std::vector<vk::ImageView> mImageViewHandles;
		uint32_t mWidth;
		uint32_t mHeight;
	};

} // namespace graphics