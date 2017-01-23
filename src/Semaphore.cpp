#include "Semaphore.h"

namespace vksp
{

	Semaphore::Semaphore(const DeviceRef &tDevice) :
		mDevice(tDevice)
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		auto result = vkCreateSemaphore(mDevice->getHandle(), &semaphoreCreateInfo, nullptr, &mSemaphoreHandle);
		assert(result == VK_SUCCESS);
	}

	Semaphore::~Semaphore()
	{
		vkDestroySemaphore(mDevice->getHandle(), mSemaphoreHandle, nullptr);
	}

} // namespace vksp