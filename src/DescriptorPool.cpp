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

	DescriptorPool::DescriptorPool(const DeviceRef& device, const std::vector<vk::DescriptorPoolSize>& descriptor_pool_sizes, uint32_t max_sets) :
		m_device(device)
	{
		vk::DescriptorPoolCreateInfo descriptor_pool_create_info;
		descriptor_pool_create_info.maxSets = max_sets;
		descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes.size());
		descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes.data();

		m_descriptor_pool_handle = m_device->get_handle().createDescriptorPool(descriptor_pool_create_info);
	}

} // namespace graphics