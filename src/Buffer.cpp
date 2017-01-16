#include "Buffer.h"

namespace vk
{
	
	Buffer::Options::Options()
	{

	}

	Buffer::Buffer(const DeviceRef &tDevice, size_t tSize, const void *tData, const Options &tOptions) :
		mDevice(tDevice),
		mSize(tSize),
		mQueueFamilyIndices(tOptions.mQueueFamilyIndices)
	{
		VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (mQueueFamilyIndices.size())
		{
			std::cout << "This buffer is used by multiple queue families: setting its share mode to VK_SHARING_MODE_CONCURRENT\n";
			sharingMode = VK_SHARING_MODE_CONCURRENT;
		}

		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.pQueueFamilyIndices = mQueueFamilyIndices.data();	// Ignored if the sharing mode is exclusive.
		bufferCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(mQueueFamilyIndices.size());
		bufferCreateInfo.sharingMode = sharingMode;
		bufferCreateInfo.size = mSize;
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		auto result = vkCreateBuffer(mDevice->getHandle(), &bufferCreateInfo, nullptr, &mBufferHandle);
		assert(result == VK_SUCCESS);

		std::cout << "Successfully created buffer\n";

		// Store the memory requirements for this buffer object.
		vkGetBufferMemoryRequirements(mDevice->getHandle(), mBufferHandle, &mMemoryRequirements);

		allocateMemory();

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

	void Buffer::allocateMemory()
	{
		auto &physicalDeviceMemoryProperties = mDevice->getPhysicalDeviceMemoryProperties();

		// Make sure that the memory type is visible to the host for reads/writes.
		VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		uint32_t selectedMemoryIndex = 0;

		for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i) 
		{
			if ((mMemoryRequirements.memoryTypeBits & (1 << i)) && 
				(physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags) 
			{
				selectedMemoryIndex = i;
				break;
			}
		}

		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.allocationSize = mMemoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = selectedMemoryIndex;
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		auto result = vkAllocateMemory(mDevice->getHandle(), &memoryAllocateInfo, nullptr, &mDeviceMemoryHandle);
		assert(result == VK_SUCCESS);

		std::cout << "Successfully allocated device memory for the buffer object\n";
	}

} // namespace vk