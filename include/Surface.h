#pragma once

#include <memory>

#include "Platform.h"
#include "Instance.h"

namespace vk
{

	class Surface;
	using SurfaceRef = std::shared_ptr<Surface>;

	class Surface
	{

	public:

		static SurfaceRef create(const InstanceRef &tInstance)
		{
			return std::make_shared<Surface>(tInstance);
		}

		Surface(const InstanceRef &tInstance);
		~Surface();

		inline VkSurfaceKHR getHandle() const { return mSurfaceHandle; }

	private:

		VkSurfaceKHR mSurfaceHandle;

		InstanceRef mInstance;
		
		friend class Window;

	};

} // namespace vk