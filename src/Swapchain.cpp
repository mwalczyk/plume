#include "Swapchain.h"

namespace vk
{

	Swapchain::Swapchain(DeviceRef tDeviceRef, PhysicalDeviceRef tPhysicalDeviceRef) :
		mDeviceRef{ tDeviceRef },
		mPhysicalDeviceRef{ tPhysicalDeviceRef }
	{

	}

	Swapchain::~Swapchain()
	{
		vkDestroySwapchainKHR(mDeviceRef->getHandle(), mSwapchainHandle, nullptr);
	}

} // namespace vk