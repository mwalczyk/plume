#include "Surface.h"

namespace graphics
{

	Surface::Surface(const InstanceRef &tInstance) :
		mInstance(tInstance)
	{

	}

	Surface::~Surface()
	{
		vkDestroySurfaceKHR(mInstance->getHandle(), mSurfaceHandle, nullptr);
	}

} // namespace graphics