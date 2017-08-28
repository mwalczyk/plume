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

#pragma once

#include <memory>
#include <vector>

#include "Platform.h"
#include "Noncopyable.h"
#include "Device.h"
#include "DeviceMemory.h"

namespace graphics
{

	class Buffer;
	using BufferRef = std::shared_ptr<Buffer>;

	//! Buffers represent linear arrays of data. They are created with a usage bitmask which describes the
	//! allowed usages of the buffer (i.e. vk::BufferUsageFlagBits::eUniformBuffer). Any combination of bits
	//! can be specified. Buffers are created with a sharing mode that controls how they can be accessed 
	//! from queues. Note that if a buffer is created with the vk::SharingMode::eExclusive sharing mode, 
	//! ownership can be transferred to another queue.
	class Buffer : public Noncopyable
	{
	public:

		static const vk::BufferUsageFlags BUFFER_USAGE_ALL;

		//! Factory method for returning a new BufferRef that will be filled with the supplied vector of data.
		template<class T>
		static BufferRef create(const DeviceRef& device, vk::BufferUsageFlags buffer_usage_flags, const std::vector<T>& data, const std::vector<Device::QueueType> queues = { Device::QueueType::GRAPHICS })
		{
			return std::make_shared<Buffer>(device, buffer_usage_flags, sizeof(T) * data.size(), data.data(), queues);
		}

		//! Factory method for returning a new BufferRef that will be filled with the supplied data.
		static BufferRef create(const DeviceRef& device, vk::BufferUsageFlags buffer_usage_flags, size_t size, const void* data, const std::vector<Device::QueueType> queues = { Device::QueueType::GRAPHICS })
		{
			return std::make_shared<Buffer>(device, buffer_usage_flags, size, data, queues);
		}

		Buffer(const DeviceRef& device, 
			vk::BufferUsageFlags buffer_usage_flags,		
			size_t size, 
			const void* data, 
			const std::vector<Device::QueueType> queues = { Device::QueueType::GRAPHICS });

		~Buffer();

		inline vk::Buffer get_handle() const { return m_buffer_handle; }
		inline DeviceMemoryRef get_device_memory() const { return m_device_memory; }
		inline vk::BufferUsageFlags get_buffer_usage_flags() const { return m_buffer_usage_flags; }

		inline vk::DescriptorBufferInfo build_descriptor_info(vk::DeviceSize offset = 0, vk::DeviceSize range = VK_WHOLE_SIZE) const 
		{ 
			return { m_buffer_handle, offset, range };  
		}

		//! Returns the size of the data that was used to construct this buffer. Note that this is not the same as the total device memory  
		//! allocation size, which can be queried from the buffer's device memory reference.
		inline size_t get_requested_size() const { return m_requested_size; }

	private:

		DeviceRef m_device;
		DeviceMemoryRef m_device_memory;
		vk::Buffer m_buffer_handle;
		vk::BufferUsageFlags m_buffer_usage_flags;
		vk::MemoryRequirements m_memory_requirements;
		size_t m_requested_size;
	};

} // namespace graphics
