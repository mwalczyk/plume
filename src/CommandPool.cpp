#include "CommandPool.h"

namespace vk
{
	
	CommandPool::Options::Options()
	{
		mCommandPoolCreateFlags = 0;
	}

	CommandPool::CommandPool(uint32_t tQueueFamilyIndex, const DeviceRef &tDevice, const Options &tOptions) :
		mQueueFamilyIndex(tQueueFamilyIndex),
		mDevice(tDevice),
		mCommandPoolCreateFlags(tOptions.mCommandPoolCreateFlags)
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo = {};
		commandPoolCreateInfo.flags = mCommandPoolCreateFlags;
		commandPoolCreateInfo.queueFamilyIndex = mQueueFamilyIndex;
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

		auto result = vkCreateCommandPool(mDevice->getHandle(), &commandPoolCreateInfo, nullptr, &mCommandPoolHandle);
		assert(result == VK_SUCCESS);

		std::cout << "Successfully created command pool\n";
	}

	CommandPool::~CommandPool()
	{
		vkDestroyCommandPool(mDevice->getHandle(), mCommandPoolHandle, nullptr);
	}

} // namespace vk