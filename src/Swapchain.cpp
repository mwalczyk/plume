#include "Swapchain.h"

namespace vksp
{
	Swapchain::Options::Options()
	{
		mPresentMode = vk::PresentModeKHR::eMailbox;
	}

	Swapchain::Swapchain(const DeviceRef &tDevice, const SurfaceRef &tSurface, uint32_t tWidth, uint32_t tHeight, const Options &tOptions) :
		mDevice(tDevice),
		mSurface(tSurface),
		mWidth(tWidth),
		mHeight(tHeight)
	{
		auto swapchainSupportDetails = mDevice->getSwapchainSupportDetails(tSurface);

		// From the structure above, determine an optimal surface format, presentation mode, and size for the swapchain.
		auto selectedSurfaceFormat = selectSwapchainSurfaceFormat(swapchainSupportDetails.mFormats);
		auto selectedPresentMode = selectSwapchainPresentMode(swapchainSupportDetails.mPresentModes);
		auto selectedExtent = selectSwapchainExtent(swapchainSupportDetails.mCapabilities);

		// If the maxImageCount field is 0, this indicates that there is no limit (besides memory requirements) to the number of images in the swapchain.
		uint32_t imageCount = swapchainSupportDetails.mCapabilities.minImageCount + 1;
		if (swapchainSupportDetails.mCapabilities.maxImageCount > 0 && imageCount > swapchainSupportDetails.mCapabilities.maxImageCount)
		{
			imageCount = swapchainSupportDetails.mCapabilities.maxImageCount;
		}

		// For now, we assume that the graphics and presentation queues are the same - this is indicated by the VK_SHARING_MODE_EXCLUSIVE flag.
		// In the future, we will need to account for the fact that these two operations may be a part of different queue families.
		vk::SwapchainCreateInfoKHR swapchainCreateInfo;
		swapchainCreateInfo.clipped = VK_TRUE;										// Make sure to ignore pixels that are obscured by other windows.
		swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;	// This window should not blend with any other windows in the windowing system.
		swapchainCreateInfo.imageArrayLayers = 1;									
		swapchainCreateInfo.imageColorSpace = selectedSurfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = selectedExtent;
		swapchainCreateInfo.imageFormat = selectedSurfaceFormat.format;
		swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;			// This swapchain is only accessed by one queue family (see notes above).
		swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
		swapchainCreateInfo.minImageCount = imageCount;
		swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;							// If the sharing mode is exlusive, we don't need to specify this.
		swapchainCreateInfo.presentMode = selectedPresentMode;
		swapchainCreateInfo.preTransform = swapchainSupportDetails.mCapabilities.currentTransform;
		swapchainCreateInfo.queueFamilyIndexCount = 0;								// Again, if the sharing mode is exlusive, we don't need to specify this.
		swapchainCreateInfo.surface = mSurface->getHandle();

		mSwapchainHandle = mDevice->getHandle().createSwapchainKHR(swapchainCreateInfo);

		// Note that the Vulkan implementation may create more swapchain images than requested above - this is why we query the number of images again.
		mImageHandles = mDevice->getHandle().getSwapchainImagesKHR(mSwapchainHandle);
	
		// Store the image format and extent for later use.
		mSwapchainImageFormat = selectedSurfaceFormat.format;
		mSwapchainImageExtent = selectedExtent;

		createImageViews();
	}

	Swapchain::~Swapchain()
	{
		mDevice->getHandle().destroySwapchainKHR(mSwapchainHandle);
	}

	uint32_t Swapchain::acquireNextSwapchainImage(const SemaphoreRef &tSemaphore, uint32_t tNanosecondsTimeout)
	{
		auto result = mDevice->getHandle().acquireNextImageKHR(mSwapchainHandle, tNanosecondsTimeout, tSemaphore->getHandle(), VK_NULL_HANDLE);
		return result.value;
	}

	vk::SurfaceFormatKHR Swapchain::selectSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &tSurfaceFormats) const
	{
		// If there is only one VkSurfaceFormatKHR entry with format vk::Format::eUndefined, this means that the surface has no preferred format,
		// in which case we default to vk::Format::eB8G8R8A8Unorm and vk::ColorSpaceKHR::eSrgbNonlinear.
		if (tSurfaceFormats.size() == 1 &&
			tSurfaceFormats[0].format == vk::Format::eUndefined)
		{
			return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
		}

		// Otherwise, there is a preferred format - iterate through and see if the above combination is available.
		for (const auto &surfaceFormat : tSurfaceFormats)
		{
			if (surfaceFormat.format == vk::Format::eB8G8R8A8Unorm &&
				surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
			{
				return surfaceFormat;
			}
		}

		// At this point, we could start ranking the available formats and determine which one is "best."
		// For now, return the first available format, since our preferred format was not available.
		return tSurfaceFormats[0];
	}

	vk::PresentModeKHR Swapchain::selectSwapchainPresentMode(const std::vector<vk::PresentModeKHR> &tPresentModes) const
	{
		// The swapchain can use one of the following modes for presentation:
		// vk::PresentModeKHR::eImmediate
		// vk::PresentModeKHR::eFifo (the only mode guaranteed to be available)
		// vk::PresentModeKHR::eFifoRelaxed
		// vk::PresentModeKHR::eMailbox
		for (const auto& presentMode : tPresentModes)
		{
			if (presentMode == vk::PresentModeKHR::eMailbox)
			{
				return presentMode;
			}
		}
		
		// This present mode is always available - use it if the preferred mode is not found.
		return vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D Swapchain::selectSwapchainExtent(const vk::SurfaceCapabilitiesKHR &tSurfaceCapabilities) const
	{
		if (tSurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return tSurfaceCapabilities.currentExtent;
		}
		else
		{
			vk::Extent2D actualExtent = { mWidth, mHeight };
			actualExtent.width = std::max(tSurfaceCapabilities.minImageExtent.width, std::min(tSurfaceCapabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(tSurfaceCapabilities.minImageExtent.height, std::min(tSurfaceCapabilities.maxImageExtent.height, actualExtent.height));
			return actualExtent;
		}
	}

	void Swapchain::createImageViews()
	{
		mImageViewHandles.resize(mImageHandles.size());

		for (size_t i = 0; i < mImageViewHandles.size(); ++i)
		{
			vk::ImageViewCreateInfo imageViewCreateInfo;
			imageViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;					// For now, do not swizzle any of the color channels.
			imageViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
			imageViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
			imageViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
			imageViewCreateInfo.format = mSwapchainImageFormat;
			imageViewCreateInfo.image = mImageHandles[i];
			imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;	// This describes the image's purpose - we will be using these images as color targets.
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;							// This describes which part of the image we will access.
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.viewType = vk::ImageViewType::e2D;								// Treat the image as a standard 2D texture.

			mImageViewHandles[i] = mDevice->getHandle().createImageView(imageViewCreateInfo);
		}
	}

} // namespace vksp