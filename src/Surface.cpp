#include "Surface.h"

namespace vk
{

	Surface::Surface(const InstanceRef &tInstance) :
		mInstance(tInstance)
	{

	}

	Surface::~Surface()
	{
		vkDestroySurfaceKHR(mInstance->getHandle(), mSurfaceHandle, nullptr);
	}

} // namespace vk