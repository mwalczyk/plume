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

#include "DescriptorPool.h"

namespace graphics
{

	DescriptorPool::DescriptorPool(DeviceWeakRef device, const std::vector<vk::DescriptorPoolSize>& descriptor_pool_sizes, uint32_t max_sets) :
		m_device(device),
		m_descriptor_pool_sizes(descriptor_pool_sizes),
		m_max_sets(max_sets),
		m_available_sets(max_sets)
	{
		DeviceRef device_shared = m_device.lock();

		// Fill out the container that maps each descriptor type to the number of descriptors of that type
		// that can be allocated from this descriptor pool across ALL future descriptor sets. This will be 
		// used to track allocations from this pool.
		for (const auto& descriptor_pool_size : m_descriptor_pool_sizes)
		{
			auto type = descriptor_pool_size.type;
			auto count = descriptor_pool_size.descriptorCount;

			m_descriptor_type_to_count_available_mapping.at(type) += count;
		}

		vk::DescriptorPoolCreateInfo descriptor_pool_create_info;
		descriptor_pool_create_info.maxSets = max_sets;
		descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes.size());
		descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes.data();

		m_descriptor_pool_handle = device_shared->get_handle().createDescriptorPool(descriptor_pool_create_info);
	}

	DescriptorPool::~DescriptorPool()
	{
		DeviceRef device_shared = m_device.lock();

		device_shared->get_handle().destroyDescriptorPool(m_descriptor_pool_handle);
	}

} // namespace graphics