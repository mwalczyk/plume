#include "Surface.h"

namespace vksp
{

	Surface::Surface(const InstanceRef &tInstance) :
		mInstance(tInstance)
	{

	}

	Surface::~Surface()
	{
		vkDestroySurfaceKHR(mInstance->getHandle(), mSurfaceHandle, nullptr);
	}

} // namespace vksp