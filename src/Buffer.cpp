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
		vkBindBufferMemory(mDevice->getHandle(), mBufferHandle, mDeviceMemory->getHandle(), 0);
	}

	Buffer::~Buffer()
	{
		vkDestroyBuffer(mDevice->getHandle(), mBufferHandle, nullptr);
	}

} // namespace graphics