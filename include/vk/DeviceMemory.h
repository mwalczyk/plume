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

#include "Platform.h"
#include "Noncopyable.h"
#include "Device.h"

namespace graphics
{

	class DeviceMemory;
	using DeviceMemoryRef = std::shared_ptr<DeviceMemory>;

	//! Device memory is memory that is visible to the device. The memory properties of a physical device 
	//! describe the memory heaps and memory types available. Each heap describes a memory resource of
	//! a particular size, and each memory type describes a set of memory properties (e.g. host cached vs.
	//! uncached) that can be used with a given memory heap.
	//!
	//! Before freeing any object bound to device memory, the application must ensure that the memory 
	//! object is no longer in use by the device - for example, by command buffers queued for execution. 
	//!
	//! Memory objects created with the vk::MemoryPropertyFlagBits::eHostVisible bit are considered 
	//! mappable.
	class DeviceMemory : public Noncopyable
	{
	public:
		
		//! Factory method for returning a new DeviceMemoryRef.
		static DeviceMemoryRef create(DeviceWeakRef device, const vk::MemoryRequirements& memory_requirements, vk::MemoryPropertyFlags required_memory_properties)
		{
			return std::make_shared<DeviceMemory>(device, memory_requirements, required_memory_properties);
		}

		//! Construct a stack allocated, non-copyable container that manages a device memory allocation.
		DeviceMemory(DeviceWeakRef device, const vk::MemoryRequirements& memory_requirements, vk::MemoryPropertyFlags required_memory_properties);
		
		~DeviceMemory();

		vk::DeviceMemory get_handle() const { return m_device_memory_handle; }

		vk::DeviceSize get_allocation_size() const { return m_allocation_size; }

		uint32_t get_selected_memory_index() const { return m_selected_memory_index; }

		//! Retrieve a host virtual address pointer to a region of this memory allocation. Note that this
		//! function does not check whether any previously submitted commands that accessed the memory
		//! region have completed. While a range of device memory is mapped for host access, the application
		//! is responsible for synchronizing both device and host access to that memory range. If the memory 
		//! is not coherent, a flush command must be used to guarantee that host writes are visible to the
		//! device.
		void* map(size_t offset, size_t size);

		//! Unmaps the memory object.
		void unmap();

	private:

		DeviceWeakRef m_device;
		vk::DeviceMemory m_device_memory_handle;
		vk::DeviceSize m_allocation_size;
		uint32_t m_selected_memory_index;
		bool m_in_use;
	};

} // namespace graphics