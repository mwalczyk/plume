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
#include <vector>

#include "Platform.h"
#include "Noncopyable.h"
#include "Device.h"
#include "DeviceMemory.h"

namespace graphics
{

	class Buffer;
	using BufferRef = std::shared_ptr<Buffer>;

	class Buffer : public Noncopyable
	{
	public:

		struct Options
		{
			Options();

			Options& queueFamilyIndices(const std::vector<uint32_t> tQueueFamilyIndices) { mQueueFamilyIndices = tQueueFamilyIndices; return *this; }
			Options& useStagingBuffer(bool tUseStagingBuffer) { mUseStagingBuffer = tUseStagingBuffer; return *this; }

			std::vector<uint32_t> mQueueFamilyIndices;
			bool mUseStagingBuffer;
		};

		//! Factory method for returning a new BufferRef.
		template<class T>
		static BufferRef create(const DeviceRef &tDevice, vk::BufferUsageFlags tBufferUsageFlags, const std::vector<T> &tData, const Options &tOptions = Options())
		{
			return std::make_shared<Buffer>(tDevice, tBufferUsageFlags, sizeof(T) * tData.size(), tData.data());
		}

		static BufferRef create(const DeviceRef &tDevice, vk::BufferUsageFlags tBufferUsageFlags, size_t tSize, const void *tData, const Options &tOptions = Options())
		{
			return std::make_shared<Buffer>(tDevice, tBufferUsageFlags, tSize, tData, tOptions);
		}

		Buffer(const DeviceRef &tDevice, vk::BufferUsageFlags tBufferUsageFlags, size_t tSize, const void *tData, const Options &tOptions = Options());
		~Buffer();

		inline vk::Buffer getHandle() const { return mBufferHandle; }
		inline DeviceMemoryRef getDeviceMemory() const { return mDeviceMemory; }
		inline vk::BufferUsageFlags getBufferUsageFlags() const { return mBufferUsageFlags; }
		inline vk::DescriptorBufferInfo buildDescriptorInfo(vk::DeviceSize tOffset = 0, vk::DeviceSize tRange = VK_WHOLE_SIZE) const { return { mBufferHandle, tOffset, tRange };  }

		//! Returns the size of the data that was used to construct this buffer. Note that this is not the same as the total device memory  
		//! allocation size, which can be queried from the buffer's device memory reference.
		inline size_t getRequestedSize() const { return mRequestedSize; }

	private:

		DeviceRef mDevice;
		DeviceMemoryRef mDeviceMemory;
		vk::Buffer mBufferHandle;
		vk::BufferUsageFlags mBufferUsageFlags;
		size_t mRequestedSize;
	};

} // namespace graphics
