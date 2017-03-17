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
#include <map>
#include <vector>

#include "Platform.h"
#include "Noncopyable.h"
#include "Device.h"

namespace graphics
{

	class DescriptorPool;
	using DescriptorPoolRef = std::shared_ptr<DescriptorPool>;

	//! Descriptor pools maintain a pool of descriptors, from which descriptor sets are allocated. They are 
	//! constructed by specifying one or more descriptor pool size structs, each of which contains a descriptor
	//! type (i.e. vk::DescriptorType::eUniformBuffer) and a descriptor count. The descriptor pool will 
	//! allocate enough storage for the total number of descriptors of each type.
	class DescriptorPool : public Noncopyable
	{
	public:

		//! Factory method for returning a new DescriptorPoolRef. Takes a list of descriptor pool size structs, each of
		//! which maps a descriptor type to a descriptor count. For example, a pool that is large enough to hold 3 uniform  
		//! buffers, 4 combined image samplers, and 1 storage buffer could be created like:
		//!			
		//!		auto pool = graphics::DescriptorPool::create(device, { { vk::DescriptorType::eUniformBuffer, 3 }, 
		//!															   { vk::DescriptorType::eCombinedImageSampler, 4},
		//!															   { vk::DescriptorType::eStorageBuffer, 1} });
		//!
		static DescriptorPoolRef create(const DeviceRef& device, const std::vector<vk::DescriptorPoolSize>& descriptor_pool_sizes, uint32_t max_sets = 1)
		{

			return std::make_shared<DescriptorPool>(device, descriptor_pool_sizes, max_sets);
		}

		DescriptorPool(const DeviceRef& device, const std::vector<vk::DescriptorPoolSize>& descriptor_pool_sizes, uint32_t max_sets = 1);

		inline vk::DescriptorPool get_handle() const { return m_descriptor_pool_handle; }

	public:

		DeviceRef m_device;
		vk::DescriptorPool m_descriptor_pool_handle;
	};

} // namespace graphics