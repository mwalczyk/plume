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
#include "Log.h"

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
		static BufferRef create(DeviceWeakRef device, vk::BufferUsageFlags buffer_usage_flags, const std::vector<T>& data, const std::vector<QueueType> queues = { QueueType::GRAPHICS })
		{
			return std::make_shared<Buffer>(device, buffer_usage_flags, sizeof(T) * data.size(), data.data(), queues);
		}

		//! Factory method for returning a new BufferRef that will be filled with the supplied data.
		static BufferRef create(DeviceWeakRef device, vk::BufferUsageFlags buffer_usage_flags, size_t size, const void* data, const std::vector<QueueType> queues = { QueueType::GRAPHICS })
		{
			return std::make_shared<Buffer>(device, buffer_usage_flags, size, data, queues);
		}

		Buffer(DeviceWeakRef device,
			vk::BufferUsageFlags buffer_usage_flags,		
			size_t size, 
			const void* data, 
			const std::vector<QueueType> queues = { QueueType::GRAPHICS });

		~Buffer();

		DeviceMemoryRef get_device_memory() const { return m_device_memory; }

		vk::Buffer get_handle() const { return m_buffer_handle; }

		//! Returns the usage flags that were used to create this buffer (i.e. vertex buffer, index buffer, uniform buffer, etc.).
		vk::BufferUsageFlags get_buffer_usage_flags() const { return m_buffer_usage_flags; }

		//! Returns the memory requirements of this buffer resource, as reported by the driver. Used
		//! to allocate the device memory associated with this buffer.
		const vk::MemoryRequirements& get_memory_requirements() const { return m_memory_requirements; }

		//! Returns the size of the data that was used to construct this buffer. Note that this is not the same as the total device memory  
		//! allocation size, which can be queried from the buffer's device memory reference.
		size_t get_requested_size() const { return m_requested_size; }

		//! Uploads data to the buffer's device memory region. Note that if the device memory associated with this buffer is not marked
		//! as vk::MemoryPropertyFlagBits::eHostCoherent, then you must use a flush command after writing to the memory.
		template<class T>
		void upload_immediately(const T* data, vk::DeviceSize offset = 0)
		{
			void* mapped_ptr = m_device_memory->map();
			memcpy(mapped_ptr, data + offset, sizeof(T));
			m_device_memory->unmap();

			// TODO: if the device memory associated with this buffer is not host coherent, we need to flush.
		}

		//! Returns a vk::DescriptorBufferInfo for this buffer object. By default, `offset` is set to zero, and `range` is set to
		//! the special value VK_WHOLE_SIZE, meaning that the descriptor will access the entire extent of this buffer's memory.
		vk::DescriptorBufferInfo build_descriptor_info(vk::DeviceSize offset = 0, vk::DeviceSize range = VK_WHOLE_SIZE) const
		{
			// See: https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkDescriptorBufferInfo.html
			if (range != VK_WHOLE_SIZE &&
				range < 0 ||
				range <= m_device_memory->get_allocation_size() - offset)
			{
				throw std::runtime_error("Invalid value for `range` parameter of `build_descriptor_info()`");
			}

			return{ m_buffer_handle, offset, range };
		}

	private:

		DeviceWeakRef m_device;
		DeviceMemoryRef m_device_memory;
		vk::Buffer m_buffer_handle;
		vk::BufferUsageFlags m_buffer_usage_flags;
		vk::MemoryRequirements m_memory_requirements;
		size_t m_requested_size;
	};

} // namespace graphics
