/*
*
* MIT License
*
* Copyright(c) 2017 Michael Walczyk
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files(the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions :
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*/

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