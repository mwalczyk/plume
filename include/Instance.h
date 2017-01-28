#pragma once

#include <memory>
#include <vector>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <functional>

#include "Platform.h"
#include "Noncopyable.h"

namespace graphics
{
	
	class Instance;
	using InstanceRef = std::shared_ptr<Instance>;

	class Instance : public Noncopyable
	{
	public:

		class Options
		{
		public:
			
			Options();

			//! Specify the names of all instance layers that should be enabled. By default,
			//! only the VK_LAYER_LUNARG_standard_validation layer is enabled.
			Options& requiredLayers(const std::vector<const char*> tRequiredLayers) { mRequiredLayers = tRequiredLayers; return *this; }

			//! Add a single name to the list of instance layers that should be enabled.
			Options& appendRequiredLayer(const char* tLayerName) { mRequiredLayers.push_back(tLayerName); return *this; }

			//! Specify the names of all instance extensions that should be enabled.
			Options& requiredExtensions(const std::vector<const char*> tRequiredExtensions) { mRequiredExtensions = tRequiredExtensions; return *this; }
			
			//! Add a single name to the list of instance extensions that should be enabled. By default,
			//! only the VK_EXT_debug_report instance extension is enabled.
			Options& appendRequiredExtensions(const char* tExtensionName) { mRequiredExtensions.push_back(tExtensionName); return *this; }
			
			//! Specify a complete VkApplicationInfo structure that will be used to create this instance.
			Options& applicationInfo(const vk::ApplicationInfo &tApplicationInfo) { mApplicationInfo = tApplicationInfo; return *this; }
			
		private:

			std::vector<const char*> mRequiredLayers;
			std::vector<const char*> mRequiredExtensions;
			vk::ApplicationInfo mApplicationInfo;

			friend class Instance;
		};

		//! Factory method for returning a new InstanceRef.
		static InstanceRef create(const Options &tOptions = Options()) { return std::make_shared<Instance>(tOptions); }

		Instance(const Options &tOptions = Options());
		~Instance();

		inline vk::Instance getHandle() const { return mInstanceHandle; }
		inline const std::vector<vk::ExtensionProperties>& getInstanceExtensionProperties() const { return mInstanceExtensionProperties; }
		inline const std::vector<vk::LayerProperties>& getInstanceLayerProperties() const { return mInstanceLayerProperties; }
		inline const std::vector<vk::PhysicalDevice>& getPhysicalDevices() const { return mPhysicalDevices; }
		vk::PhysicalDevice pickPhysicalDevice(const std::function<bool(vk::PhysicalDevice)> &tCandidacyFunc);

	private:

		bool checkInstanceLayerSupport();
		void setupDebugReportCallback();

		vk::Instance mInstanceHandle;
		VkDebugReportCallbackEXT mDebugReportCallback;
		std::vector<vk::ExtensionProperties> mInstanceExtensionProperties;
		std::vector<vk::LayerProperties> mInstanceLayerProperties;
		std::vector<vk::PhysicalDevice> mPhysicalDevices;
		std::vector<const char*> mRequiredLayers;
		std::vector<const char*> mRequiredExtensions;

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
	};

} // namespace graphics