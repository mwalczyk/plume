#pragma once

#include <memory>

#include "Platform.h"
#include "Device.h"

namespace vksp
{

	class CommandPool;
	using CommandPoolRef = std::shared_ptr<CommandPool>;

	class CommandPool
	{
	public:

		class Options
		{
		public:

			Options();

			//! Determines how and when individual command buffers allocated from this pool can be re-recorded. 
			//! Possible flags are VK_COMMAND_POOL_CREATE_TRANSIENT_BIT and VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT.
			Options& commandPoolCreateFlags(VkCommandPoolCreateFlags tCommandPoolCreateFlags) { mCommandPoolCreateFlags = tCommandPoolCreateFlags; return *this; }

		private:

			VkCommandPoolCreateFlags mCommandPoolCreateFlags;

			friend class CommandPool;
		};

		//! Factory method for returning a new CommandPoolRef.
		static CommandPoolRef create(const DeviceRef &tDevice, uint32_t tQueueFamilyIndex, const Options &tOptions = Options())
		{
			return std::make_shared<CommandPool>(tDevice, tQueueFamilyIndex, tOptions);
		}

		CommandPool(const DeviceRef &tDevice, uint32_t tQueueFamilyIndex, const Options &tOptions = Options());
		~CommandPool();

		inline VkCommandPool getHandle() const { return mCommandPoolHandle; };

	private:

		VkCommandPool mCommandPoolHandle;

		DeviceRef mDevice;
	};

} // namespace vksp