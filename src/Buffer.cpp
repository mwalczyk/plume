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

#include "Buffer.h"

namespace graphics
{
	
	Buffer::Options::Options()
	{
		mUseStagingBuffer = false;
	}

	Buffer::Buffer(const DeviceRef &tDevice, vk::BufferUsageFlags tBufferUsageFlags, size_t tSize, const void *tData, const Options &tOptions) :
		mDevice(tDevice),
		mBufferUsageFlags(tBufferUsageFlags),
		mRequestedSize(tSize)
	{
		vk::SharingMode sharingMode = vk::SharingMode::eExclusive;
		if (tOptions.mQueueFamilyIndices.size())
		{
			std::cout << "This buffer is used by multiple queue families: setting its share mode to VK_SHARING_MODE_CONCURRENT\n";
			sharingMode = vk::SharingMode::eConcurrent;
		}

		vk::BufferCreateInfo bufferCreateInfo;
		bufferCreateInfo.pQueueFamilyIndices = tOptions.mQueueFamilyIndices.data();	// Ignored if the sharing mode is exclusive.
		bufferCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(tOptions.mQueueFamilyIndices.size());
		bufferCreateInfo.sharingMode = sharingMode;
		bufferCreateInfo.size = mRequestedSize;
		bufferCreateInfo.usage = mBufferUsageFlags;

		if (tOptions.mUseStagingBuffer)
		{
			// Steps:
			// Create a second command pool for command buffers that are submitted on the transfer queue family.
			// Change the sharingMode of resources to be VK_SHARING_MODE_CONCURRENT and specify both the graphics and transfer queue families.
			// Submit the transfer command vkCmdCopyBuffer to the transfer queue instead of the graphics queue.

			// If there is a separate transfer queue available on this device, use it for setting up the staging buffer. The buffer needs to be
			// created with this in mind. First, see if it was already included in the list of queue family indices that was passed to the constructor.
			// If it wasn't, add it to the new list below, which will be used to create both buffers.
			auto transferIndex = mDevice->getQueueFamiliesMapping().transfer().second;
			std::vector<uint32_t> stagedQueueFamilyIndices(tOptions.mQueueFamilyIndices.begin(), tOptions.mQueueFamilyIndices.end());

			if (std::find(stagedQueueFamilyIndices.begin(), stagedQueueFamilyIndices.end(), transferIndex) == stagedQueueFamilyIndices.end())
			{
				stagedQueueFamilyIndices.push_back(transferIndex);
			}
		}
		else
		{
			mBufferHandle = mDevice->getHandle().createBuffer(bufferCreateInfo);
		}

		// Store the memory requirements for this buffer object.
		auto memoryRequirements = mDevice->getHandle().getBufferMemoryRequirements(mBufferHandle);
		auto requiredMemoryProperties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

		// Allocate device memory.
		mDeviceMemory = DeviceMemory::create(mDevice, memoryRequirements, requiredMemoryProperties);

		// Fill the buffer with the data that was passed into the constructor.
		if (tData)
		{
			void* mappedPtr = mDeviceMemory->map(0, mDeviceMemory->getAllocationSize());
			memcpy(mappedPtr, tData, static_cast<size_t>(mRequestedSize));
			mDeviceMemory->unmap();
		}

		// Associate the device memory with this buffer object.
		mDevice->getHandle().bindBufferMemory(mBufferHandle, mDeviceMemory->getHandle(), 0);
	}

	Buffer::~Buffer()
	{
		mDevice->getHandle().destroyBuffer(mBufferHandle);
	}

} // namespace graphics