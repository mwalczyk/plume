#include "Buffer.h"

namespace vk
{
	
	Buffer::Options::Options()
	{
		mUseStagingBuffer = false;
	}

	Buffer::Buffer(const DeviceRef &tDevice, VkBufferUsageFlags tBufferUsageFlags, size_t tSize, const void *tData, const Options &tOptions) :
		mDevice(tDevice),
		mBufferUsageFlags(tBufferUsageFlags),
		mSize(tSize)
	{
		VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (tOptions.mQueueFamilyIndices.size())
		{
			std::cout << "This buffer is used by multiple queue families: setting its share mode to VK_SHARING_MODE_CONCURRENT\n";
			sharingMode = VK_SHARING_MODE_CONCURRENT;
		}

		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.pQueueFamilyIndices = tOptions.mQueueFamilyIndices.data();	// Ignored if the sharing mode is exclusive.
		bufferCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(tOptions.mQueueFamilyIndices.size());
		bufferCreateInfo.sharingMode = sharingMode;
		bufferCreateInfo.size = mSize;
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
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
			auto transferIndex = mDevice->getQueueFamilyIndices().mTransferIndex;
			std::vector<uint32_t> stagedQueueFamilyIndices(tOptions.mQueueFamilyIndices.begin(), tOptions.mQueueFamilyIndices.end());

			if (std::find(stagedQueueFamilyIndices.begin(), stagedQueueFamilyIndices.end(), transferIndex) == stagedQueueFamilyIndices.end())
			{
				stagedQueueFamilyIndices.push_back(transferIndex);
			}
		}
		else
		{
			auto result = vkCreateBuffer(mDevice->getHandle(), &bufferCreateInfo, nullptr, &mBufferHandle);
			assert(result == VK_SUCCESS);
		}

		std::cout << "Successfully created buffer\n";

		// Store the memory requirements for this buffer object.
		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(mDevice->getHandle(), mBufferHandle, &memoryRequirements);

		allocateMemory(memoryRequirements);

		// Fill the buffer with the data that was passed into the constructor.
		void* mappedPtr = map(0, mSize);
		memcpy(mappedPtr, tData, static_cast<size_t>(mSize));
		unmap();

		// Associate the device memory with this buffer object.
		vkBindBufferMemory(mDevice->getHandle(), mBufferHandle, mDeviceMemoryHandle, 0);
	}

	Buffer::~Buffer()
	{
		vkDestroyBuffer(mDevice->getHandle(), mBufferHandle, nullptr);
		vkFreeMemory(mDevice->getHandle(), mDeviceMemoryHandle, nullptr);
	}

	void* Buffer::map(size_t tOffset, size_t tSize)
	{
		assert(tOffset < mSize && tSize <= mSize);
		void* mappedPtr;
		vkMapMemory(mDevice->getHandle(), mDeviceMemoryHandle, tOffset, tSize, 0, &mappedPtr); // Consider using VK_WHOLE_SIZE here.
		return mappedPtr;
	}

	void Buffer::unmap()
	{
		vkUnmapMemory(mDevice->getHandle(), mDeviceMemoryHandle);
	}

	uint32_t Buffer::findMemoryTypeIndex(uint32_t tMemoryTypeBits, VkMemoryPropertyFlags tMemoryPropertyFlags)
	{
		auto &physicalDeviceMemoryProperties = mDevice->getPhysicalDeviceMemoryProperties();

		uint32_t selectedMemoryIndex = 0;
		for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
		{
			if ((tMemoryTypeBits & (1 << i)) &&
				(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & tMemoryPropertyFlags) == tMemoryPropertyFlags)
			{
				selectedMemoryIndex = i;
				break;
			}
		}

		return selectedMemoryIndex;
	}

	void Buffer::allocateMemory(const VkMemoryRequirements &tMemoryRequirements)
	{
		// Make sure that the memory type is visible to the host for reads/writes.
		uint32_t memoryTypeIndex = findMemoryTypeIndex(tMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.allocationSize = tMemoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		auto result = vkAllocateMemory(mDevice->getHandle(), &memoryAllocateInfo, nullptr, &mDeviceMemoryHandle);
		assert(result == VK_SUCCESS);

		std::cout << "Successfully allocated device memory for the buffer object\n";
	}

} // namespace vk