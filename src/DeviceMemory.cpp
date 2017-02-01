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

namespace graphics
{

	DeviceMemory::DeviceMemory(const DeviceRef& device, const vk::MemoryRequirements& memory_requirements, vk::MemoryPropertyFlags required_memory_properties) :
		m_device(device),
		m_selected_memory_index(0),
		m_allocation_size(memory_requirements.size)
	{
		auto& physical_device_memory_properties = m_device->get_physical_device_memory_properties();

		// Based on the memory requirements, find the index of the memory heap that should be used to allocate memory.
		for (uint32_t i = 0; i < physical_device_memory_properties.memoryTypeCount; ++i)
		{
			if ((memory_requirements.memoryTypeBits & (1 << i)) &&
				physical_device_memory_properties.memoryTypes[i].propertyFlags & required_memory_properties)
			{
				m_selected_memory_index = i;
				break;
			}
		}

		vk::MemoryAllocateInfo memory_allocate_info{ memory_requirements.size, m_selected_memory_index };
		
		m_device_memory_handle = m_device->get_handle().allocateMemory(memory_allocate_info);
	}

	DeviceMemory::~DeviceMemory()
	{
		if (m_in_use)
		{
			unmap();
		}
		m_device->get_handle().freeMemory(m_device_memory_handle);
	}

	void* DeviceMemory::map(size_t offset, size_t size)
	{
		if (offset > m_allocation_size && size <= m_allocation_size)
		{
			return nullptr;
		}
		void* mapped_ptr = m_device->get_handle().mapMemory(m_device_memory_handle, offset, size);
		m_in_use = true;
		return mapped_ptr;
	}

	void DeviceMemory::unmap()
	{
		m_device->get_handle().unmapMemory(m_device_memory_handle);
		m_in_use = false;
	}

} // namespace graphics