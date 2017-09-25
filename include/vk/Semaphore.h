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

namespace graphics
{

	class Semaphore;
	using SemaphoreRef = std::shared_ptr<Semaphore>;

	//! Semaphores are a synchronization primitive that can be used to insert a dependency between batches
	//! submitted to queues. A semaphore can be in one of two states: signaled or unsignaled. Semaphores can be
	//! used during the queue submission process to signal that a batch of work has completed (i.e. rendering
	//! has finished). They can also be used to delay the start of a batch of work (i.e. presenting an image to 
	//! the screen should not start until rendering has finished).
	class Semaphore : public Noncopyable
	{
	public:

		//! Factory method for returning a new SemaphoreRef.
		static SemaphoreRef create(DeviceWeakRef device)
		{
			return std::make_shared<Semaphore>(device);
		}

		Semaphore(DeviceWeakRef device);
		
		~Semaphore();

		vk::Semaphore get_handle() const { return m_semaphore_handle; };

	private:

		DeviceWeakRef m_device;
		vk::Semaphore m_semaphore_handle;
	};

} // namespace graphics