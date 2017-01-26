#include "DeviceMemory.h"

namespace graphics
{

	DeviceMemory::DeviceMemory(const DeviceRef &tDevice, const vk::MemoryRequirements &tMemoryRequirements, vk::MemoryPropertyFlags tRequiredMemoryProperties) :
		mDevice(tDevice),
		mSelectedMemoryIndex(0),
		mAllocationSize(tMemoryRequirements.size)
	{
		auto &physicalDeviceMemoryProperties = mDevice->getPhysicalDeviceMemoryProperties();

		// Based on the memory requirements, find the index of the memory heap that should be used to allocate memory.
		for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
		{
			if ((tMemoryRequirements.memoryTypeBits & (1 << i)) &&
				physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & tRequiredMemoryProperties)
			{
				mSelectedMemoryIndex = i;
				break;
			}
		}

		vk::MemoryAllocateInfo memoryAllocateInfo{ tMemoryRequirements.size, mSelectedMemoryIndex };
		
		mDeviceMemoryHandle = mDevice->getHandle().allocateMemory(memoryAllocateInfo);
	}

	DeviceMemory::~DeviceMemory()
	{
		mDevice->getHandle().freeMemory(mDeviceMemoryHandle);
	}

	void* DeviceMemory::map(size_t tOffset, size_t tSize)
	{
		if (tOffset > mAllocationSize && tSize <= mAllocationSize)
		{
			return nullptr;
		}
		void* mappedPtr = mDevice->getHandle().mapMemory(mDeviceMemoryHandle, tOffset, tSize);
		return mappedPtr;
	}

	void DeviceMemory::unmap()
	{
		mDevice->getHandle().unmapMemory(mDeviceMemoryHandle);
	}

} // namespace graphics