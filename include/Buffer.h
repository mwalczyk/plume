#pragma once

#include <memory>
#include <vector>

#include "Platform.h"
#include "Device.h"

namespace vk
{

	class Buffer;
	using BufferRef = std::shared_ptr<Buffer>;

	class Buffer
	{

	public:

		struct VertexAttribute
		{
			uint32_t mBinding;
			uint32_t mLocation;
			VkFormat mFormat;
		};

		struct VertexData
		{
			VertexData(uint32_t tBinding, VkVertexInputRate tVertexInputRate, uint32_t tStride) :
				mBinding(tBinding),
				mVertexInputRate(tVertexInputRate),
				mStride(tStride)
			{
			}

			uint32_t mBinding;
			VkVertexInputRate mVertexInputRate;
			uint32_t mStride;
			std::pair<uint32_t, VertexAttribute> mVertexAttributes;

			VkVertexInputBindingDescription mVertexInputBindingDescription;
			std::vector<VkVertexInputAttributeDescription> mVertexInputAttributeDescriptions;
		};

		struct Options
		{
		
		};

		//! Factory method for returning a new BufferRef.
		static BufferRef create(const DeviceRef &tDevice, const Options &tOptions = Options())
		{
			return std::make_shared<Buffer>(tDevice, tOptions);
		}

		Buffer(const DeviceRef &tDevice, const Options &tOptions = Options());
		~Buffer();

		inline VkBuffer getHandle() const { return mBufferHandle; }
		inline VkDeviceMemory getDeviceMemoryHandle() const { return mDeviceMemoryHandle; }

	private:

		//! Based on the memory requirements of this buffer, find the index of the memory heap that should be used to back this buffer.
		void allocateMemory();

		VkBuffer mBufferHandle;
		VkDeviceMemory mDeviceMemoryHandle;
		VkMemoryRequirements mMemoryRequirements;

		DeviceRef mDevice;

	};

} // namespace vk
