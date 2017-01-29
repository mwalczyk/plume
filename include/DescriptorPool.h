#pragma once

#include <memory>
#include <map>
#include <vector>

#include "Platform.h"
#include "Noncopyable.h"
#include "Device.h"

namespace graphics
{

	class DescriptorPool;
	using DescriptorPoolRef = std::shared_ptr<DescriptorPool>;

	//! Descriptor pools maintain a pool of descriptors, from which descriptor sets are allocated. They are 
	//! constructed by specifying one or more descriptor pool size structs, each of which contains a descriptor
	//! type (i.e. vk::DescriptorType::eUniformBuffer) and a descriptor count. The descriptor pool will 
	//! allocate enough storage for the total number of descriptors of each type.
	class DescriptorPool : public Noncopyable
	{
	public:

		//! Factory method for returning a new DescriptorPoolRef. Takes a list of descriptor pool size structs, each of
		//! which maps a descriptor type to a descriptor count. For example, a pool that is large enough to hold 3 uniform  
		//! buffers, 4 combined image samplers, and 1 storage buffer could be created like:
		//!			
		//!		auto pool = graphics::DescriptorPool::create(device, {{ vk::DescriptorType::eUniformBuffer, 3 }, 
		//!															  { vk::DescriptorType::eCombinedImageSampler, 4},
		//!															  { vk::DescriptorType::eStorageBuffer, 1}});
		//!
		static DescriptorPoolRef create(const DeviceRef &tDevice, const std::vector<vk::DescriptorPoolSize> &tDescriptorPoolSizes, uint32_t tMaxSets = 1)
		{

			return std::make_shared<DescriptorPool>(tDevice, tDescriptorPoolSizes, tMaxSets);
		}

		DescriptorPool(const DeviceRef &tDevice, const std::vector<vk::DescriptorPoolSize> &tDescriptorPoolSizes, uint32_t tMaxSets = 1);

		inline vk::DescriptorPool getHandle() const { return mDescriptorPoolHandle; }

	public:

		DeviceRef mDevice;
		vk::DescriptorPool mDescriptorPoolHandle;
	};

} // namespace graphics