#pragma once

#include <memory>
#include <vector>
#include <cassert>

#include "vulkan/vulkan.h"

namespace vk
{

	class Instance;
	using InstanceRef = std::shared_ptr<Instance>;

	class Instance
	{

	public:

		struct Options
		{
			Options();

			Options& requiredLayers(const std::vector<const char*> tRequiredLayers) { mRequiredLayers = tRequiredLayers; return *this; }
			Options& requiredExtensions(const std::vector<const char*> tRequiredExtensions) { mRequiredExtensions = tRequiredExtensions; return *this; }
			Options& applicationInfo(const VkApplicationInfo &tApplicationInfo) { mApplicationInfo = tApplicationInfo; return *this; }
			
			std::vector<const char*> mRequiredLayers;
			std::vector<const char*> mRequiredExtensions;
			VkApplicationInfo mApplicationInfo;
		};

		Instance(const Options &tOptions = Options());
		~Instance();

		inline VkInstance getHandle() const { return mInstanceHandle; }

	private:

		bool checkInstanceLayerSupport();

		VkInstance mInstanceHandle;

		std::vector<const char*> mRequiredLayers;
		std::vector<const char*> mRequiredExtensions;
		VkApplicationInfo mApplicationInfo;

	};

} // namespace