#include "PhysicalDevice.h"

namespace vk
{

	PhysicalDevice::PhysicalDevice()
	{

	}

	PhysicalDevice::~PhysicalDevice()
	{
		// It is not necessary to destroy the physical device: they are not created with a dedicated
		// function like logical devices. Rather, they are considered to be owned by the Vulkan 
		// instance. Therefore, when the instance is destroyed, the resources associated with each
		// physical device are freed as well.
	}

} // namespace vk