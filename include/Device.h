#pragma once

#include <memory>

#include "vulkan/vulkan.h"

namespace vk
{

	class Device;
	using DeviceRef = std::shared_ptr<Device>;

	class Device
	{

	public:

		Device();
		~Device();

		inline VkDevice getHandle() const { return mDeviceHandle; };

	private:

		VkDevice mDeviceHandle;

	};

} // namespace vk