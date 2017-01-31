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

	class CommandPool : public Noncopyable
	{
	public:

		class Options
		{
		public:

			Options();

			//! Determines how and when individual command buffers allocated from this pool can be re-recorded. 
			//! Possible flags are vk::CommandPoolCreateFlagBits::eTransient and vk::CommandPoolCreateFlagBits::eResetCommandBuffer.
			Options& commandPoolCreateFlags(vk::CommandPoolCreateFlags tCommandPoolCreateFlags) { mCommandPoolCreateFlags = tCommandPoolCreateFlags; return *this; }
			
		private:

			vk::CommandPoolCreateFlags mCommandPoolCreateFlags;

			friend class CommandPool;
		};

		//! Factory method for returning a new CommandPoolRef.
		static CommandPoolRef create(const DeviceRef &tDevice, uint32_t tQueueFamilyIndex, const Options &tOptions = Options())
		{
			return std::make_shared<CommandPool>(tDevice, tQueueFamilyIndex, tOptions);
		}

		CommandPool(const DeviceRef &tDevice, uint32_t tQueueFamilyIndex, const Options &tOptions = Options());
		~CommandPool();

		inline vk::CommandPool getHandle() const { return mCommandPoolHandle; };

	private:

		DeviceRef mDevice;
		vk::CommandPool mCommandPoolHandle;
	};

} // namespace graphics