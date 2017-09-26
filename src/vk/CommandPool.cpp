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

namespace plume
{

	namespace graphics
	{

		CommandPool::CommandPool(const Device& device, QueueType queue_type, vk::CommandPoolCreateFlags command_pool_create_flags) :

			m_device_ptr(&device)
		{
			vk::CommandPoolCreateInfo command_pool_create_info;
			command_pool_create_info.flags = command_pool_create_flags;
			command_pool_create_info.queueFamilyIndex = m_device_ptr->get_queue_family_index(queue_type);

			m_command_pool_handle = m_device_ptr->get_handle().createCommandPoolUnique(command_pool_create_info);
		}

	} // namespace graphics

} // namespace plume