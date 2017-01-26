#pragma once

#include <memory>

#include "Platform.h"
#include "Noncopyable.h"
#include "Device.h"

namespace graphics
{

	class DeviceMemory;
	using DeviceMemoryRef = std::shared_ptr<DeviceMemory>;

	class DeviceMemory : public Noncopyable
	{
	public:

		//! Factory method for returning a new DeviceMemoryRef.
		static DeviceMemoryRef create(const DeviceRef &tDevice, const vk::MemoryRequirements &tMemoryRequirements, vk::MemoryPropertyFlags tRequiredMemoryProperties)
		{
			return std::make_shared<DeviceMemory>(tDevice, tMemoryRequirements, tRequiredMemoryProperties);
		}

		//! Construct a stack allocated, non-copyable container that manages a device memory allocation.
		DeviceMemory(const DeviceRef &tDevice, const vk::MemoryRequirements &tMemoryRequirements, vk::MemoryPropertyFlags tRequiredMemoryProperties);
		~DeviceMemory();

		inline vk::DeviceMemory getHandle() const { return mDeviceMemoryHandle; }
		inline vk::DeviceSize getAllocationSize() const { return mAllocationSize; }
		inline uint32_t getSelectedMemoryIndex() const { return mSelectedMemoryIndex; }
		void* map(size_t tOffset, size_t tSize);
		void unmap();

	private:

		DeviceRef mDevice;
		vk::DeviceMemory mDeviceMemoryHandle;
		vk::DeviceSize mAllocationSize;
		uint32_t mSelectedMemoryIndex;
	};

} // namespace graphics