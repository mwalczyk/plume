#include "DescriptorPool.h"

namespace graphics
{

	DescriptorPool::DescriptorPool(const DeviceRef &tDevice, const std::vector<vk::DescriptorPoolSize> &tDescriptorPoolSizes, uint32_t tMaxSets) :
		mDevice(tDevice)
	{
		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
		descriptorPoolCreateInfo.maxSets = tMaxSets;
		descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(tDescriptorPoolSizes.size());
		descriptorPoolCreateInfo.pPoolSizes = tDescriptorPoolSizes.data();

		mDescriptorPoolHandle = mDevice->getHandle().createDescriptorPool(descriptorPoolCreateInfo);
	}

} // namespace graphics