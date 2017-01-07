#include "Instance.h"

namespace vk
{

	Instance::Options::Options()
	{
		mRequiredLayers = { "VK_LAYER_LUNARG_standard_validation" };
		mRequiredExtensions = { "VK_EXT_debug_report" };
		
		mApplicationInfo = {};
		mApplicationInfo.apiVersion = VK_API_VERSION_1_0;
		mApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		mApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		mApplicationInfo.pApplicationName = "Application Name";
		mApplicationInfo.pEngineName = "Engine Name";
		mApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	}

	//! Proxy function for creating a debug callback object
	VkResult createDebugReportCallbackEXT(VkInstance tInstance, const VkDebugReportCallbackCreateInfoEXT* tCreateInfo, const VkAllocationCallbacks* tAllocator, VkDebugReportCallbackEXT* tCallback)
	{
		auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(tInstance, "vkCreateDebugReportCallbackEXT");

		if (func != nullptr)
		{
			return func(tInstance, tCreateInfo, tAllocator, tCallback);
		}
		else
		{
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	//! Proxy function for destroying a debug callback object
	void destroyDebugReportCallbackEXT(VkInstance tInstance, VkDebugReportCallbackEXT tCallback, const VkAllocationCallbacks* tAllocator)
	{
		auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(tInstance, "vkDestroyDebugReportCallbackEXT");

		if (func != nullptr)
		{
			func(tInstance, tCallback, tAllocator);
		}
	}

	Instance::Instance(const Options &tOptions) :
		mRequiredLayers(tOptions.mRequiredLayers),
		mRequiredExtensions(tOptions.mRequiredExtensions),
		mApplicationInfo(tOptions.mApplicationInfo)
	{
		// Store the instance extension properties.
		uint32_t instanceExtensionPropertiesCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionPropertiesCount, nullptr);

		mInstanceExtensionProperties.resize(instanceExtensionPropertiesCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionPropertiesCount, mInstanceExtensionProperties.data());

		// Store the instance layer properties.
		uint32_t instanceLayerCount = 0;
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);

		mInstanceLayerProperties.resize(instanceLayerCount);
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, mInstanceLayerProperties.data());

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

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(mRequiredExtensions.size());
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(mRequiredLayers.size());
		instanceCreateInfo.pApplicationInfo = &mApplicationInfo;
		instanceCreateInfo.ppEnabledExtensionNames = mRequiredExtensions.data();
		instanceCreateInfo.ppEnabledLayerNames = mRequiredLayers.data();
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

		auto result = vkCreateInstance(&instanceCreateInfo, nullptr, &mInstanceHandle);
		assert(result == VK_SUCCESS);

		setupDebugReportCallback();

		// Store the handles to each of the present physical devices (note that this needs to happen after initialization).
		uint32_t physicalDeviceCount = 0;
		vkEnumeratePhysicalDevices(mInstanceHandle, &physicalDeviceCount, nullptr);

		mPhysicalDevices.resize(physicalDeviceCount);
		vkEnumeratePhysicalDevices(mInstanceHandle, &physicalDeviceCount, mPhysicalDevices.data());

		std::cout << "Successfully created instance\n";
	}

	Instance::~Instance()
	{
		destroyDebugReportCallbackEXT(mInstanceHandle, mDebugReportCallback, nullptr);

		vkDestroyInstance(mInstanceHandle, nullptr);
	}

	bool Instance::checkInstanceLayerSupport()
	{
		for (const auto& requiredLayerName: mRequiredLayers)
		{
			auto predicate = [&](const VkLayerProperties &layerProperty) { return strcmp(requiredLayerName, layerProperty.layerName) == 0; };
			if (std::find_if(mInstanceLayerProperties.begin(), mInstanceLayerProperties.end(), predicate) == mInstanceLayerProperties.end())
			{
				std::cout << "Required layer " << requiredLayerName << " is not supported\n";
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
		
		auto result = createDebugReportCallbackEXT(mInstanceHandle, &debugReportCallbackCreateInfo, nullptr, &mDebugReportCallback);
		assert(result == VK_SUCCESS);
	}

} // namespace vk