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
		m_use_staging_buffer = false;
	}

	Buffer::Buffer(const DeviceRef& device, vk::BufferUsageFlags buffer_usage_flags, size_t size, const void* data, const Options& options) :
		m_device(device),
		m_buffer_usage_flags(buffer_usage_flags),
		m_requested_size(size)
	{
		vk::SharingMode sharing_mode = vk::SharingMode::eExclusive;
		if (options.m_queue_family_indices.size())
		{
			std::cout << "This buffer is used by multiple queue families: setting its share mode to VK_SHARING_MODE_CONCURRENT\n";
			sharing_mode = vk::SharingMode::eConcurrent;
		}

		vk::BufferCreateInfo buffer_create_info;
		buffer_create_info.pQueueFamilyIndices = options.m_queue_family_indices.data();	// Ignored if the sharing mode is exclusive.
		buffer_create_info.queueFamilyIndexCount = static_cast<uint32_t>(options.m_queue_family_indices.size());
		buffer_create_info.sharingMode = sharing_mode;
		buffer_create_info.size = m_requested_size;
		buffer_create_info.usage = m_buffer_usage_flags;

		if (options.m_use_staging_buffer)
		{
			// Steps:
			// Create a second command pool for command buffers that are submitted on the transfer queue family.
			// Change the sharingMode of resources to be VK_SHARING_MODE_CONCURRENT and specify both the graphics and transfer queue families.
			// Submit the transfer command vkCmdCopyBuffer to the transfer queue instead of the graphics queue.

			// If there is a separate transfer queue available on this device, use it for setting up the staging buffer. The buffer needs to be
			// created with this in mind. First, see if it was already included in the list of queue family indices that was passed to the constructor.
			// If it wasn't, add it to the new list below, which will be used to create both buffers.
			auto transfer_index = m_device->get_queue_families_mapping().transfer().second;
			std::vector<uint32_t> staged_queue_family_indices(options.m_queue_family_indices.begin(), options.m_queue_family_indices.end());

			if (std::find(staged_queue_family_indices.begin(), staged_queue_family_indices.end(), transfer_index) == staged_queue_family_indices.end())
			{
				staged_queue_family_indices.push_back(transfer_index);
			}
		}
		else
		{
			m_buffer_handle = m_device->get_handle().createBuffer(buffer_create_info);
		}

		// Store the memory requirements for this buffer object.
		auto memory_requirements = m_device->get_handle().getBufferMemoryRequirements(m_buffer_handle);
		auto required_memory_properties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

		// Allocate device memory.
		m_device_memory = DeviceMemory::create(m_device, memory_requirements, required_memory_properties);

		// Fill the buffer with the data that was passed into the constructor.
		if (data)
		{
			void* mapped_ptr = m_device_memory->map(0, m_device_memory->get_allocation_size());
			memcpy(mapped_ptr, data, static_cast<size_t>(m_requested_size));
			m_device_memory->unmap();
		}

		// Associate the device memory with this buffer object.
		m_device->get_handle().bindBufferMemory(m_buffer_handle, m_device_memory->get_handle(), 0);
	}

	Buffer::~Buffer()
	{
		m_device->get_handle().destroyBuffer(m_buffer_handle);
	}

} // namespace graphics