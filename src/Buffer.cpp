#include "Buffer.h"

namespace vk
{
	
	static const float vertices[] = {
	  0.0f, -0.5f, 1.0f, 0.0f, 0.0f,
	  0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
	 -0.5f, 0.5f, 0.0f, 0.0f, 1.0f
	};

	Buffer::Buffer(const DeviceRef &tDevice, const Options &tOptions) :
		mDevice(tDevice)
	{
		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.pQueueFamilyIndices = nullptr; // Ignored if the sharing mode is exclusive.
		bufferCreateInfo.queueFamilyIndexCount = 0;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferCreateInfo.size = sizeof(vertices);
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		auto result = vkCreateBuffer(mDevice->getHandle(), &bufferCreateInfo, nullptr, &mBufferHandle);
		assert(result == VK_SUCCESS);

		std::cout << "Successfully created buffer\n";

		// Store the memory requirements for this buffer object.
		vkGetBufferMemoryRequirements(mDevice->getHandle(), mBufferHandle, &mMemoryRequirements);
		std::cout << "Buffer memory requirements:\n";
		std::cout << "\tRequired alignment: " << mMemoryRequirements.alignment << "\n";
		std::cout << "\tRequired size: " << mMemoryRequirements.size << "\n";

		allocateMemory();

		// Associate the device memory with this buffer object.
		vkBindBufferMemory(mDevice->getHandle(), mBufferHandle, mDeviceMemoryHandle, 0);

		// Fill the buffer.
		void* data;
		vkMapMemory(mDevice->getHandle(), mDeviceMemoryHandle, 0, bufferCreateInfo.size, 0, &data);
		memcpy(data, vertices, static_cast<size_t>(bufferCreateInfo.size));
		vkUnmapMemory(mDevice->getHandle(), mDeviceMemoryHandle);

		std::cout << "Buffer size: " << sizeof(vertices) << std::endl;
	}

	Buffer::~Buffer()
	{
		vkDestroyBuffer(mDevice->getHandle(), mBufferHandle, nullptr);
		vkFreeMemory(mDevice->getHandle(), mDeviceMemoryHandle, nullptr);
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