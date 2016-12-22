#include "Instance.h"

namespace vk
{

	Instance::Options::Options()
	{
		mRequiredLayers = { "VK_LAYER_LUNARG_standard_validation" };
		mRequiredExtensions = { "VK_EXT_debug_report" };
		
		mApplicationInfo.apiVersion = VK_API_VERSION_1_0;
		mApplicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		mApplicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		mApplicationInfo.pApplicationName = "";
		mApplicationInfo.pEngineName = "";
		mApplicationInfo.pNext = nullptr;
		mApplicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	}

	Instance::Instance(const Options &tOptions) :
		mInstanceHandle(VK_NULL_HANDLE),
		mRequiredLayers(tOptions.mRequiredLayers),
		mRequiredExtensions(tOptions.mRequiredExtensions),
		mApplicationInfo(tOptions.mApplicationInfo)
	{
		if (!checkInstanceLayerSupport())
		{
			throw std::runtime_error("one or more of the requested validation layers are not supported on this platform");
		}

		VkInstanceCreateInfo instanceCreateInfo;
		instanceCreateInfo.enabledExtensionCount = mRequiredExtensions.size();
		instanceCreateInfo.enabledLayerCount = mRequiredLayers.size();
		instanceCreateInfo.flags = 0;
		instanceCreateInfo.pApplicationInfo = &mApplicationInfo;
		instanceCreateInfo.pNext = nullptr;
		instanceCreateInfo.ppEnabledExtensionNames = mRequiredExtensions.data();
		instanceCreateInfo.ppEnabledLayerNames = mRequiredLayers.data();
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

		auto result = vkCreateInstance(&instanceCreateInfo, nullptr, &mInstanceHandle);
		assert(result == VK_SUCCESS);
	}

	Instance::~Instance()
	{
		vkDestroyInstance(mInstanceHandle, nullptr);
	}

	bool Instance::checkInstanceLayerSupport()
	{
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const auto& layerName: mRequiredLayers)
		{
			bool layerFound = false;
			for (const auto& layerProperties: availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}
		}

		return true;
	}

} // namespace vk