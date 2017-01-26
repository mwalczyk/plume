#pragma once

#include <memory>

#include "Platform.h"
#include "Device.h"

namespace graphics
{

	class Semaphore;
	using SemaphoreRef = std::shared_ptr<Semaphore>;

	class Semaphore
	{
	public:

		//! Factory method for returning a new SemaphoreRef.
		static SemaphoreRef create(const DeviceRef &tDevice)
		{
			return std::make_shared<Semaphore>(tDevice);
		}

		Semaphore(const DeviceRef &tDevice);
		~Semaphore();

		inline VkSemaphore getHandle() const { return mSemaphoreHandle; };

	private:

		VkSemaphore mSemaphoreHandle;

		DeviceRef mDevice;
	};

} // namespace graphics