#pragma once

#include <memory>
#include <vector>

#include "vulkan/vulkan.h"
#include "Device.h"

namespace vk
{

	class Swapchain;
	using SwapchainRef = std::shared_ptr<Swapchain>;

	class Swapchain
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

		//! Factory method for returning a new SwapchainRef.
		static SwapchainRef create(const DeviceRef &tDevice, const SurfaceRef &tSurface, const Options &tOptions = Options())
		{
			return std::make_shared<Swapchain>(tDevice, tSurface, tOptions);
		}

		Swapchain(const DeviceRef &tDevice, const SurfaceRef &tSurface, const Options &tOptions = Options());
		~Swapchain();

		inline VkSwapchainKHR getHandle() const { return mSwapchainHandle; };

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
		std::vector<VkImage> mSwapchainImages;
		std::vector<VkImageView> mSwapchainImageViews;
		std::vector<VkFramebuffer> mSwapchainFramebuffers;

		uint32_t mWidth;
		uint32_t mHeight;
	};

} // namespace vk