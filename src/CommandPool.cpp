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

#include "CommandPool.h"

namespace graphics
{
	
	CommandPool::Options::Options()
	{
		m_command_pool_create_flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	}

	CommandPool::CommandPool(const DeviceRef& device, uint32_t queue_family_index, const Options& options) :
		m_device(device)
	{
		vk::CommandPoolCreateInfo command_pool_create_info;
		command_pool_create_info.flags = options.m_command_pool_create_flags;
		command_pool_create_info.queueFamilyIndex = queue_family_index;

		m_command_pool_handle = m_device->getHandle().createCommandPool(command_pool_create_info);
	}

	CommandPool::~CommandPool()
	{
		m_device->getHandle().destroyCommandPool(m_command_pool_handle);
	}

} // namespace graphics