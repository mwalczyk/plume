#pragma once

#include <memory>

#include "vulkan/vulkan.h"

namespace vk
{

	class PhysicalDevice;
	using PhysicalDeviceRef = std::shared_ptr<PhysicalDevice>;

	class PhysicalDevice
	{

	public:

		PhysicalDevice() = default;
		~PhysicalDevice();

		inline VkPhysicalDevice getHandle() const { return mPhysicalDeviceHandle; };

	private:

		VkPhysicalDevice mPhysicalDeviceHandle;

	};

} // namespace vk