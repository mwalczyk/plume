#include "Device.h"

namespace vk
{

	Device::Device()
	{

	}

	Device::~Device()
	{
		vkDestroyDevice(mDeviceHandle, nullptr);
	}

} // namespace vk