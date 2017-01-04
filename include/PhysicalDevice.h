#pragma once

#include <memory>
#include <vector>
#include <cassert>

#include "vulkan/vulkan.h"

namespace vk
{

	class PhysicalDevice;
	using PhysicalDeviceRef = std::shared_ptr<PhysicalDevice>;

	class PhysicalDevice
	{

	public:

		PhysicalDevice();
		~PhysicalDevice();

		inline VkPhysicalDevice getHandle() const { return mPhysicalDeviceHandle; };
		std::vector<VkExtensionProperties> getDeviceExtensionProperties() const;

	private:

		VkPhysicalDevice mPhysicalDeviceHandle;

	};

} // namespace vk