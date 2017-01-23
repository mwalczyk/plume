#include "Instance.h"

namespace vksp
{

	Instance::Options::Options()
	{
		mRequiredLayers = { "VK_LAYER_LUNARG_standard_validation" };
		mRequiredExtensions = { "VK_EXT_debug_report" };
		
		mApplicationInfo = {};
		mApplicationInfo.apiVersion = VK_API_VERSION_1_0;
		mApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		mApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		mApplicationInfo.pApplicationName = "Spectra Application";
		mApplicationInfo.pEngineName = "Spectra Engine";
	}

	//! Proxy function for creating a debug callback object
	vk::Result createDebugReportCallbackEXT(VkInstance tInstance, const VkDebugReportCallbackCreateInfoEXT* tCreateInfo, const VkAllocationCallbacks* tAllocator, VkDebugReportCallbackEXT* tCallback)
	{
		auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(tInstance, "vkCreateDebugReportCallbackEXT");

		if (func != nullptr)
		{
			return static_cast<vk::Result>(func(tInstance, tCreateInfo, tAllocator, tCallback));
		}
		else
		{
			return vk::Result::eErrorExtensionNotPresent;
		}
	}

	//! Proxy function for destroying a debug callback object
	void destroyDebugReportCallbackEXT(vk::Instance tInstance, VkDebugReportCallbackEXT tCallback, const VkAllocationCallbacks* tAllocator)
	{
		auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(tInstance, "vkDestroyDebugReportCallbackEXT");

		if (func != nullptr)
		{
			func(static_cast<VkInstance>(tInstance), tCallback, tAllocator);
		}
	}

	Instance::Instance(const Options &tOptions) :
		mRequiredLayers(tOptions.mRequiredLayers),
		mRequiredExtensions(tOptions.mRequiredExtensions)
	{
		// Store the instance extension properties.
		mInstanceExtensionProperties = vk::enumerateInstanceExtensionProperties();

		// Store the instance layer properties.
		mInstanceLayerProperties = vk::enumerateInstanceLayerProperties();

		if (!checkInstanceLayerSupport())
		{
			throw std::runtime_error("One or more of the requested validation layers are not supported on this platform");
		}

		// Append the instance extensions required by the windowing system
#if defined(SPECTRA_MSW)
		mRequiredExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(SPECTRA_LINUX)
		mRequiredExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
		mRequiredExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

		// Create the instance.
		vk::InstanceCreateInfo instanceCreateInfo;
		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(mRequiredExtensions.size());
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(mRequiredLayers.size());
		instanceCreateInfo.pApplicationInfo = &tOptions.mApplicationInfo;
		instanceCreateInfo.ppEnabledExtensionNames = mRequiredExtensions.data();
		instanceCreateInfo.ppEnabledLayerNames = mRequiredLayers.data();

		mInstanceHandle = vk::createInstance(instanceCreateInfo);

		// Only set up the debug callback object if the VK_EXT_debug_report extension is present.
		if (std::find(mRequiredExtensions.begin(), mRequiredExtensions.end(), "VK_EXT_debug_report") != mRequiredExtensions.end())
		{
			setupDebugReportCallback();
		}

		// Store the handles to each of the present physical devices (note that this needs to happen after initialization).
		mPhysicalDevices = mInstanceHandle.enumeratePhysicalDevices();
	}

	Instance::~Instance()
	{
		destroyDebugReportCallbackEXT(mInstanceHandle, mDebugReportCallback, nullptr);
		
		mInstanceHandle.destroy();
	}

	vk::PhysicalDevice Instance::pickPhysicalDevice(const std::function<bool(vk::PhysicalDevice)> &tCandidacyFunc)
	{
		for (const auto &physicalDevice : mPhysicalDevices)
		{
			if (tCandidacyFunc(physicalDevice))
			{
				return physicalDevice;
			}
		}
		
		return VK_NULL_HANDLE;
	}

	bool Instance::checkInstanceLayerSupport()
	{
		for (const auto& requiredLayerName: mRequiredLayers)
		{
			auto predicate = [&](const vk::LayerProperties &layerProperty) { return strcmp(requiredLayerName, layerProperty.layerName) == 0; };
			if (std::find_if(mInstanceLayerProperties.begin(), mInstanceLayerProperties.end(), predicate) == mInstanceLayerProperties.end())
			{
				std::cerr << "Required layer " << requiredLayerName << " is not supported\n";
				return false;
			}
		}

		return true;
	}

	void Instance::setupDebugReportCallback()
	{
		VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = {};
		debugReportCallbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		debugReportCallbackCreateInfo.pfnCallback = debugCallback;
		debugReportCallbackCreateInfo.pUserData = nullptr;
		debugReportCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		
		createDebugReportCallbackEXT(mInstanceHandle, &debugReportCallbackCreateInfo, nullptr, &mDebugReportCallback);
	}

} // namespace vksp