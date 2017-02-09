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
		m_memory_property_flags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
	}

	Buffer::Buffer(const DeviceRef& device, vk::BufferUsageFlags buffer_usage_flags, size_t size, const void* data, const Options& options) :
		m_device(device),
		m_buffer_usage_flags(buffer_usage_flags),
		m_requested_size(size)
	{
		vk::SharingMode sharing_mode = vk::SharingMode::eExclusive;
		if (options.m_queue_family_indices.size())
		{
			std::cout << "This buffer is used by multiple queue families: setting its share mode to vk::SharingMode::eConcurrent\n";
			sharing_mode = vk::SharingMode::eConcurrent;
		}

		vk::BufferCreateInfo buffer_create_info;
		buffer_create_info.pQueueFamilyIndices = options.m_queue_family_indices.data();	// Ignored if the sharing mode is exclusive.
		buffer_create_info.queueFamilyIndexCount = static_cast<uint32_t>(options.m_queue_family_indices.size());
		buffer_create_info.sharingMode = sharing_mode;
		buffer_create_info.size = m_requested_size;
		buffer_create_info.usage = m_buffer_usage_flags;

		m_buffer_handle = m_device->get_handle().createBuffer(buffer_create_info);

		// Store the memory requirements for this buffer object.
		m_memory_requirements = m_device->get_handle().getBufferMemoryRequirements(m_buffer_handle);

		// Allocate device memory.
		m_device_memory = DeviceMemory::create(m_device, m_memory_requirements, options.m_memory_property_flags);

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