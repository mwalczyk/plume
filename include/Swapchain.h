#pragma once

#include <memory>
#include <vector>

#include "vulkan/vulkan.h"
#include "Device.h"
#include "PhysicalDevice.h"

namespace vk
{

	class Swapchain;
	using SwapchainRef = std::shared_ptr<Swapchain>;

	class Swapchain
	{

	public:

		Swapchain() = default;
		Swapchain(DeviceRef tDeviceRef, PhysicalDeviceRef tPhysicalDeviceRef);
		~Swapchain();

		struct SwapchainSupportDetails
		{
			VkSurfaceCapabilitiesKHR mCapabilities;
			std::vector<VkSurfaceFormatKHR> mFormats;		
			std::vector<VkPresentModeKHR> mPresentModes;	
		};

		inline VkSwapchainKHR getHandle() const { return mSwapchainHandle; };

	private:

		DeviceRef mDeviceRef;
		PhysicalDeviceRef mPhysicalDeviceRef;

		VkSwapchainKHR mSwapchainHandle;

		VkFormat mSwapchainImageFormat;
		VkExtent2D mSwapchainImageExtent;
		std::vector<VkImage> mSwapchainImages;
		std::vector<VkImageView> mSwapchainImageViews;
		std::vector<VkFramebuffer> mSwapchainFramebuffers;

		SwapchainSupportDetails mSwapchainSupportDetails;
	};

} // namespace vk