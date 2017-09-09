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
	class CommandPool;
	using CommandPoolRef = std::shared_ptr<CommandPool>;

	//! Command pools are opaque objects that command buffer memory is allocated from, and which allow the 
	//! implementation to reduce the cost of resource creation across multiple command buffers. Command pools
	//! should not be used concurrently by multiple threads. This includes any recording commands issued to 
	//! command buffers from the pool, as well as operations that allocate, free, and/or reset command 
	//! buffers or the pool itself.
	class CommandPool : public Noncopyable
	{
	public:

		//! Factory method for returning a new CommandPoolRef. The vk::CommandPoolCreateFlags parameter   
		//! determines how and when individual command buffers allocated from this pool can be re-recorded. 
		//! Possible flags are:
		//!
		//! vk::CommandPoolCreateFlagBits::eTransient: command buffers allocated from this pool will be 
		//!		short lived (reset or freed in a relatively short timeframe).
		//!
		//! vk::CommandPoolCreateFlagBits::eResetCommandBuffer: controls whether command buffers allocated
		//!		from this pool can be individually reset. Note that if this flag is not set, then all 
		//!		command buffers must be reset together.
		//!
		//! Both flags will be set by default.
		static CommandPoolRef create(DeviceWeakRef device, QueueType queue_type, vk::CommandPoolCreateFlags command_pool_create_flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient)
		{
			return std::make_shared<CommandPool>(device, queue_type, command_pool_create_flags);
		}

		CommandPool(DeviceWeakRef device, QueueType queue_type, vk::CommandPoolCreateFlags command_pool_create_flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient);
		
		~CommandPool();

		vk::CommandPool get_handle() const { return m_command_pool_handle; };

		// TODO: this should notify all command buffers that have been allocated from this pool, which means
		// that the command pool class needs to maintain a list of all command buffer objects.
		void reset_pool() 
		{ 
			DeviceRef device_shared = m_device.lock();

			device_shared->get_handle().resetCommandPool(m_command_pool_handle, vk::CommandPoolResetFlagBits::eReleaseResources);
		}

	private:

		DeviceWeakRef m_device;
		vk::CommandPool m_command_pool_handle;
	};

} // namespace graphics