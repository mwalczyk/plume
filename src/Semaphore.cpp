#include "Semaphore.h"

namespace vk
{

	Semaphore::Semaphore(const DeviceRef &tDevice) :
		mDevice(tDevice)
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		auto result = vkCreateSemaphore(mDevice->getHandle(), &semaphoreCreateInfo, nullptr, &mSemaphoreHandle);
		assert(result == VK_SUCCESS);

		std::cout << "Successfully created semaphore\n";
	}

	Semaphore::~Semaphore()
	{
		vkDestroySemaphore(mDevice->getHandle(), mSemaphoreHandle, nullptr);
	}

} // namespace vk