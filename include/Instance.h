#pragma once

#include <memory>
#include <vector>
#include <cassert>
#include <iostream>
#include <algorithm>

#include "Platform.h"

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

		//! Factory method for returning a new InstanceRef
		static InstanceRef create(const Options &tOptions = Options()) { return std::make_shared<Instance>(tOptions); }

		Instance(const Options &tOptions = Options());
		~Instance();

		inline VkInstance getHandle() const { return mInstanceHandle; }
		std::vector<VkExtensionProperties> getInstanceExtensionProperties() const;
		std::vector<VkLayerProperties> getInstanceLayerProperties() const;
		std::vector<VkPhysicalDevice> getPhysicalDevices() const;

	private:

		bool checkInstanceLayerSupport();
		void setupDebugReportCallback();

		VkInstance mInstanceHandle;
		VkDebugReportCallbackEXT mDebugReportCallback;

		std::vector<const char*> mRequiredLayers;
		std::vector<const char*> mRequiredExtensions;
		VkApplicationInfo mApplicationInfo;

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags,
															VkDebugReportObjectTypeEXT objType,
															uint64_t obj,
															size_t location,
															int32_t code,
															const char* layerPrefix,
															const char* msg,
															void* userData)
		{
			std::cerr << "VALIDATION LAYER: " << msg << std::endl;
			return VK_FALSE;
		}

		friend class Device;

	};

} // namespace