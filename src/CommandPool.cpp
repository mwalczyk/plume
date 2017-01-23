#include "CommandPool.h"

namespace vksp
{
	
	CommandPool::Options::Options()
	{
		mCommandPoolCreateFlags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	}

	CommandPool::CommandPool(const DeviceRef &tDevice, uint32_t tQueueFamilyIndex, const Options &tOptions) :
		mDevice(tDevice)
	{
		vk::CommandPoolCreateInfo commandPoolCreateInfo;
		commandPoolCreateInfo.flags = tOptions.mCommandPoolCreateFlags;
		commandPoolCreateInfo.queueFamilyIndex = tQueueFamilyIndex;

		mCommandPoolHandle = mDevice->getHandle().createCommandPool(commandPoolCreateInfo);
	}

	CommandPool::~CommandPool()
	{
		mDevice->getHandle().destroyCommandPool(mCommandPoolHandle);
	}

} // namespace vksp