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

namespace plume
{

	namespace graphics
	{

		DeviceMemory::DeviceMemory(const Device& device, const vk::MemoryRequirements& memory_requirements, vk::MemoryPropertyFlags required_memory_properties) :

			m_device_ptr(&device),
			m_memory_requirements(memory_requirements),
			m_memory_property_flags(required_memory_properties),
			m_selected_memory_index(-1),
			m_in_use(false)
		{
			find_memory_index();

			vk::MemoryAllocateInfo memory_allocate_info{ m_memory_requirements.size, m_selected_memory_index };

			m_device_memory_handle = m_device_ptr->get_handle().allocateMemoryUnique(memory_allocate_info);
		}

		DeviceMemory::~DeviceMemory()
		{
			unmap();
		}

		void DeviceMemory::find_memory_index()
		{
			auto& physical_device_memory_properties = m_device_ptr->get_physical_device_memory_properties();

			for (uint32_t i = 0; i < physical_device_memory_properties.memoryTypeCount; ++i)
			{
				// The memoryTypeBits field is a bitmask and contains one bit set for every supported memory type for the resource.
				// Bit i is set if and only if the memory type i in the physical device memory properties struct is supported for
				// this resource. The implementation guarantees that at least one bit of this bitmask will be set.
				if ((m_memory_requirements.memoryTypeBits & (1 << i)) &&
					physical_device_memory_properties.memoryTypes[i].propertyFlags & m_memory_property_flags)
				{
					m_selected_memory_index = i;
					break;
				}
			}
		}

		void* DeviceMemory::map(vk::DeviceSize offset, vk::DeviceSize size)
		{
			if (!is_host_visible())
			{
				throw std::runtime_error("Attempting to map a device memory object that is not host visible");
			}

			// It is an application error to map a memory object that is already mapped.
			if (m_in_use)
			{
				throw std::runtime_error("Attempting to map the same device memory object more than once");
			}

			if (offset > m_memory_requirements.size && size <= m_memory_requirements.size)
			{
				return nullptr;
			}

			void* mapped_ptr = m_device_ptr->get_handle().mapMemory(m_device_memory_handle.get(), offset, size);
			m_in_use = true;

			return mapped_ptr;
		}

		void DeviceMemory::unmap()
		{
			// Don't unmap the memory object if it was never in use.
			if (m_in_use)
			{
				m_device_ptr->get_handle().unmapMemory(m_device_memory_handle.get());
				m_in_use = false;
			}
		}

	} // namespace graphics

} // namespace plume