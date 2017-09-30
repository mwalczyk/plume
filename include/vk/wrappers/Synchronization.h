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

#include "Device.h"

namespace plume
{

	namespace graphics
	{

		//! Semaphores are a synchronization primitive that can be used to insert a dependency between batches
		//! submitted to queues. A semaphore can be in one of two states: signaled or unsignaled. Semaphores can be
		//! used during the queue submission process to signal that a batch of work has completed (i.e. rendering
		//! has finished). They can also be used to delay the start of a batch of work (i.e. presenting an image to 
		//! the screen should not start until rendering has finished).
		class Semaphore
		{
		public:

			Semaphore(const Device& device);

			vk::Semaphore get_handle() const { return m_semaphore_handle.get(); };

		private:

			const Device* m_device_ptr;
			vk::UniqueSemaphore m_semaphore_handle;
		};

		class Fence
		{
		public:

			Fence(const Device& device, bool create_in_signaled_state = false);

			vk::Fence get_handle() const { return m_fence_handle.get(); }

		private:

			const Device* m_device_ptr;
			vk::UniqueFence m_fence_handle;
		};

		class Event
		{
		public:

			Event(const Device& device);

			vk::Event get_handle() const { return m_event_handle.get(); }

		private:

			const Device* m_device_ptr;
			vk::UniqueEvent m_event_handle;
		};

	} // namespace graphics

} // namespace plume