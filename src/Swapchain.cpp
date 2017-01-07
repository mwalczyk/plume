#include "Swapchain.h"

namespace vk
{
	Swapchain::Options::Options()
	{
		mWidth = 640;
		mHeight = 480;
	}

	Swapchain::Swapchain(const DeviceRef &tDevice, const SurfaceRef &tSurface, const Options &tOptions) :
		mDevice(tDevice),
		mSurface(tSurface),
		mWidth(tOptions.mWidth),
		mHeight(tOptions.mHeight)
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
		std::cout << "Creating a swapchain with " << imageCount << " images\n";

		// For now, we assume that the graphics and presentation queues are the same - this is indicated by the VK_SHARING_MODE_EXCLUSIVE flag.
		// In the future, we will need to account for the fact that these two operations may be a part of different queue families.
		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
		swapchainCreateInfo.clipped = VK_TRUE;										// Make sure to ignore pixels that are obscured by other windows.
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;		// This window should not blend with any other windows in the windowing system.
		swapchainCreateInfo.imageArrayLayers = 1;									
		swapchainCreateInfo.imageColorSpace = selectedSurfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = selectedExtent;
		swapchainCreateInfo.imageFormat = selectedSurfaceFormat.format;
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;			// This swapchain is only accessed by one queue family (see notes above).
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCreateInfo.minImageCount = imageCount;
		swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;							// If the sharing mode is exlusive, we don't need to specify this.
		swapchainCreateInfo.presentMode = selectedPresentMode;
		swapchainCreateInfo.preTransform = swapchainSupportDetails.mCapabilities.currentTransform;
		swapchainCreateInfo.queueFamilyIndexCount = 0;								// Again, if the sharing mode is exlusive, we don't need to specify this.
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.surface = mSurface->getHandle();

		auto result = vkCreateSwapchainKHR(mDevice->getHandle(), &swapchainCreateInfo, nullptr, &mSwapchainHandle);
		assert(result == VK_SUCCESS);

		std::cout << "Successfully created swapchain\n";

		// Note that the Vulkan implementation may create more swapchain images than requested above - this is why we query the number of images again.
		vkGetSwapchainImagesKHR(mDevice->getHandle(), mSwapchainHandle, &imageCount, nullptr);

		mSwapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(mDevice->getHandle(), mSwapchainHandle, &imageCount, mSwapchainImages.data());

		std::cout << "Retrieved " << imageCount << " images from the swapchain\n";

		// Store the image format and extent for later use.
		mSwapchainImageFormat = selectedSurfaceFormat.format;
		mSwapchainImageExtent = selectedExtent;

		createImageViews();
	}

	Swapchain::~Swapchain()
	{
		vkDestroySwapchainKHR(mDevice->getHandle(), mSwapchainHandle, nullptr);
	}

	VkSurfaceFormatKHR Swapchain::selectSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &tSurfaceFormats) const
	{
		// If there is only one VkSurfaceFormatKHR entry with format VK_FORMAT_UNDEFINED, this means that the surface has no preferred format.
		if (tSurfaceFormats.size() == 1 &&
			tSurfaceFormats[0].format == VK_FORMAT_UNDEFINED)
		{
			std::cout << "No preferred surface format - defaulting to VK_FORMAT_B8G8R8A8_UNORM and VK_COLOR_SPACE_SRGB_NONLINEAR_KHR\n";
			return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		// Otherwise, there is a preferred format - iterate through and see if the above combination is available.
		for (const auto &surfaceFormat : tSurfaceFormats)
		{
			if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
				surfaceFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
			{
				std::cout << "Found the preferrd surface format - VK_FORMAT_B8G8R8A8_UNORM and VK_COLOR_SPACE_SRGB_NONLINEAR_KHR\n";
				return surfaceFormat;
			}
		}

		// At this point, we could start ranking the available formats and determine which one is "best."
		// For now, return the first available format, since our preferred format was not available.
		std::cout << "Could not find the preferred surface format - defaulting to the first available format\n";
		return tSurfaceFormats[0];
	}

	VkPresentModeKHR Swapchain::selectSwapchainPresentMode(const std::vector<VkPresentModeKHR> &tPresentModes) const
	{
		// The swapchain can use one of the following modes for presentation:
		// VK_PRESENT_MODE_IMMEDIATE_KHR
		// VK_PRESENT_MODE_FIFO_KHR (the only mode guaranteed to be available)
		// VK_PRESENT_MODE_FIFO_RELAXED_KHR
		// VK_PRESENT_MODE_MAILBOX_KHR

		for (const auto& presentMode : tPresentModes)
		{
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				std::cout << "Found VK_PRESENT_MODE_MAILBOX_KHR presentation mode\n";
				return presentMode;
			}
		}

		std::cout << "Presentation mode VK_PRESENT_MODE_MAILBOX_KHR is not available - defaulting to VK_PRESENT_MODE_FIFO_KHR\n";
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D Swapchain::selectSwapchainExtent(const VkSurfaceCapabilitiesKHR &tSurfaceCapabilities) const
	{
		if (tSurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			std::cout << "VkSurfaceCapabilitiesKHR currentExtent member isn't set to the maximum value of uint32_t - defaulting to:\n";
			std::cout << "\twidth: " << tSurfaceCapabilities.currentExtent.width << ", height: " << tSurfaceCapabilities.currentExtent.height << "\n";
			return tSurfaceCapabilities.currentExtent;
		}
		else
		{
			VkExtent2D actualExtent = { mWidth, mHeight };
			actualExtent.width = std::max(tSurfaceCapabilities.minImageExtent.width, std::min(tSurfaceCapabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(tSurfaceCapabilities.minImageExtent.height, std::min(tSurfaceCapabilities.maxImageExtent.height, actualExtent.height));
			std::cout << "VkSurfaceCapabilitiesKHR currentExtent member is set to the maximum value of uint32_t - setting to:\n";
			std::cout << "\twidth: " << actualExtent.width << ", height: " << actualExtent.height << "\n";
			return actualExtent;
		}
	}

	void Swapchain::createImageViews()
	{
		mSwapchainImageViews.resize(mSwapchainImages.size());

		for (size_t i = 0; i < mSwapchainImageViews.size(); ++i)
		{
			VkImageViewCreateInfo imageViewCreateInfo;
			imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;				// For now, do not swizzle any of the color channels.
			imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.flags = 0;
			imageViewCreateInfo.format = mSwapchainImageFormat;
			imageViewCreateInfo.image = mSwapchainImages[i];
			imageViewCreateInfo.pNext = nullptr;
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// This describes the image's purpose - we will be using these images as color targets.
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;						// This describes which part of the image we will access.
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;							// We want to treat the image as a standard 2D texture.

			auto result = vkCreateImageView(mDevice->getHandle(), &imageViewCreateInfo, nullptr, &mSwapchainImageViews[i]);
			assert(result == VK_SUCCESS);
		}

		std::cout << "Successfully created " << mSwapchainImageViews.size() << " image views for the swapchain\n";
	}

} // namespace vk