#pragma once

#include <memory>

#include "Platform.h"
#include "Device.h"

namespace vk
{

	class CommandPool;
	using CommandPoolRef = std::shared_ptr<CommandPool>;

	class CommandPool
	{

	public:

		struct Options
		{
			Options();

			//! Determines how and when individual command buffers allocated from this pool can be re-recorded. 
			//! Possible flags are VK_COMMAND_POOL_CREATE_TRANSIENT_BIT and VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT.
			Options& commandPoolCreateFlags(VkCommandPoolCreateFlags tCommandPoolCreateFlags) { mCommandPoolCreateFlags = tCommandPoolCreateFlags; return *this; }

			VkCommandPoolCreateFlags mCommandPoolCreateFlags;
		};

		//! Factory method for returning a new CommandPoolRef.
		static CommandPoolRef create(uint32_t tQueueFamilyIndex, const DeviceRef &tDevice, const Options &tOptions = Options())
		{
			return std::make_shared<CommandPool>(tQueueFamilyIndex, tDevice, tOptions);
		}

		CommandPool(uint32_t tQueueFamilyIndex, const DeviceRef &tDevice, const Options &tOptions = Options());
		~CommandPool();

		inline VkCommandPool getHandle() const { return mCommandPoolHandle; };

	private:

		VkCommandPool mCommandPoolHandle;

		DeviceRef mDevice;

		uint32_t mQueueFamilyIndex;
		VkCommandPoolCreateFlags mCommandPoolCreateFlags;

	};

} // namespace vk