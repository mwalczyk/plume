#pragma once

#include "Platform.h"
#include "Noncopyable.h"
#include "Device.h"

namespace vksp
{

	class DeviceMemory;
	using DeviceMemoryRef = std::shared_ptr<DeviceMemory>;

	class DeviceMemory : public Noncopyable
	{
	public:

		//! Factory method for returning a new DeviceMemoryRef.
		static DeviceMemoryRef create(const DeviceRef &tDevice, const VkMemoryRequirements &tMemoryRequirements, VkMemoryPropertyFlags tRequiredMemoryProperties)
		{
			return std::make_shared<DeviceMemory>(tDevice, tMemoryRequirements, tRequiredMemoryProperties);
		}

		//! Construct a stack allocated, non-copyable container that manages a device memory allocation.
		DeviceMemory(const DeviceRef &tDevice, const VkMemoryRequirements &tMemoryRequirements, VkMemoryPropertyFlags tRequiredMemoryProperties);
		~DeviceMemory();

		inline VkDeviceMemory getHandle() const { return mDeviceMemoryHandle; }
		inline uint32_t getSelectedMemoryIndex() const { return mSelectedMemoryIndex; }
		inline VkDeviceSize getAllocationSize() const { return mAllocationSize; }
		void* map(size_t tOffset, size_t tSize);
		void unmap();

	private:

		VkDeviceMemory mDeviceMemoryHandle;

		DeviceRef mDevice;

		uint32_t mSelectedMemoryIndex;
		VkDeviceSize mAllocationSize;
	};

} // namespace vksp