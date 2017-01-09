#include "Buffer.h"

namespace vk
{

	Buffer::Buffer(const DeviceRef &tDevice, const Options &tOptions) :
		mDevice(tDevice)
	{

	}

	Buffer::~Buffer()
	{
		vkDestroyBuffer(mDevice->getHandle(), mBufferHandle, nullptr);
		vkFreeMemory(mDevice->getHandle(), mDeviceMemoryHandle, nullptr);
	}

} // namespace vk