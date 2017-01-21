#include "CommandPool.h"

namespace vk
{
	
	CommandPool::Options::Options()
	{
		mCommandPoolCreateFlags = 0;
	}

	CommandPool::CommandPool(const DeviceRef &tDevice, uint32_t tQueueFamilyIndex, const Options &tOptions) :
		mDevice(tDevice)
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo = {};
		commandPoolCreateInfo.flags = tOptions.mCommandPoolCreateFlags;
		commandPoolCreateInfo.queueFamilyIndex = tQueueFamilyIndex;
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

		auto result = vkCreateCommandPool(mDevice->getHandle(), &commandPoolCreateInfo, nullptr, &mCommandPoolHandle);
		assert(result == VK_SUCCESS);
	}

	CommandPool::~CommandPool()
	{
		vkDestroyCommandPool(mDevice->getHandle(), mCommandPoolHandle, nullptr);
	}

} // namespace vk