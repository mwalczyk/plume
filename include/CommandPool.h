#pragma once

#include <memory>

#include "Platform.h"
#include "Noncopyable.h"
#include "Device.h"

namespace graphics
{

	class CommandPool;
	using CommandPoolRef = std::shared_ptr<CommandPool>;

	class CommandPool : public Noncopyable
	{
	public:

		class Options
		{
		public:

			Options();

			//! Determines how and when individual command buffers allocated from this pool can be re-recorded. 
			//! Possible flags are vk::CommandPoolCreateFlagBits::eTransient and vk::CommandPoolCreateFlagBits::eResetCommandBuffer.
			Options& commandPoolCreateFlags(vk::CommandPoolCreateFlags tCommandPoolCreateFlags) { mCommandPoolCreateFlags = tCommandPoolCreateFlags; return *this; }
			
		private:

			vk::CommandPoolCreateFlags mCommandPoolCreateFlags;

			friend class CommandPool;
		};

		//! Factory method for returning a new CommandPoolRef.
		static CommandPoolRef create(const DeviceRef &tDevice, uint32_t tQueueFamilyIndex, const Options &tOptions = Options())
		{
			return std::make_shared<CommandPool>(tDevice, tQueueFamilyIndex, tOptions);
		}

		CommandPool(const DeviceRef &tDevice, uint32_t tQueueFamilyIndex, const Options &tOptions = Options());
		~CommandPool();

		inline vk::CommandPool getHandle() const { return mCommandPoolHandle; };

	private:

		DeviceRef mDevice;
		vk::CommandPool mCommandPoolHandle;
	};

} // namespace graphics