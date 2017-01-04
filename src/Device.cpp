#include "Device.h"

namespace vk
{

	Device::Options::Options()
	{
		mRequiredQueueFlagBits = { VK_QUEUE_GRAPHICS_BIT };
		mRequiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		mRequiredPhysicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

		VkPhysicalDeviceFeatures defaultPhysicalDeviceFeatures;
		defaultPhysicalDeviceFeatures.tessellationShader = VK_TRUE;
		defaultPhysicalDeviceFeatures.geometryShader = VK_TRUE;

		// TODO: implement this check
		mRequiredPhysicalDeviceFeatures = defaultPhysicalDeviceFeatures;
	}

	Device::Device(const InstanceRef &tInstance, const SurfaceRef &tSurface, const Options &tOptions) :
		mInstance(tInstance),
		mSurface(tSurface),
		mRequiredQueueFlagBits(tOptions.mRequiredQueueFlagBits),
		mRequiredDeviceExtensions(tOptions.mRequiredDeviceExtensions),
		mRequiredPhysicalDeviceType(tOptions.mRequiredPhysicalDeviceType)
	{
		auto physicalDevices = mInstance->getPhysicalDevices();
		assert(physicalDevices.size() > 0);
		
		for (const auto &physicalDevice: physicalDevices)
		{
			if (isPhysicalDeviceSuitable(physicalDevice))
			{
				mPhysicalDeviceHandle = physicalDevice;
				break;
			}
		}

		// Ensure that at least one suitable GPU was found
		assert(mPhysicalDeviceHandle != VK_NULL_HANDLE);

		/*
		// enumerate the features supported by the physical device but ensure that we always have access to tessellation and geometry shaders
		auto supportedFeatures = getPhysicalDeviceFeatures(mPhysicalDeviceHandle);
		mRequiredFeatures.multiDrawIndirect = supportedFeatures.multiDrawIndirect; // only enable multi-draw indirect if it is supported
		mRequiredFeatures.tessellationShader = VK_TRUE;
		mRequiredFeatures.geometryShader = VK_TRUE;

		const float defaultQueuePriority{ 0.0f };

		// for now, we simply create a single queue from the first queue family
		VkDeviceQueueCreateInfo deviceQueueCreateInfo;
		deviceQueueCreateInfo.flags = 0;
		deviceQueueCreateInfo.pNext = nullptr;
		deviceQueueCreateInfo.pQueuePriorities = &defaultQueuePriority;
		deviceQueueCreateInfo.queueCount = 1;
		deviceQueueCreateInfo.queueFamilyIndex = 0;
		deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

		VkDeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.enabledExtensionCount = mRequiredDeviceExtensions.size();
		deviceCreateInfo.enabledLayerCount = mRequiredLayers.size();
		deviceCreateInfo.flags = 0;
		deviceCreateInfo.pEnabledFeatures = &mRequiredFeatures;
		deviceCreateInfo.pNext = nullptr;
		deviceCreateInfo.ppEnabledExtensionNames = mRequiredDeviceExtensions.data();
		deviceCreateInfo.ppEnabledLayerNames = mRequiredLayers.data();
		deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		auto result = vkCreateDevice(mPhysicalDeviceHandle, &deviceCreateInfo, nullptr, &mDeviceHandle);
		assert(result == VK_SUCCESS);

		// Store a handle to the queue
		vkGetDeviceQueue(mDeviceHandle, mQueueFamilyIndex, 0, &mQueueHandle);
		*/

		std::cout << "Sucessfully created logical and physical devices\n";
	}

	Device::~Device()
	{
		// The logical device is likely to be the last object created (aside from objects used at
		// runtime). Before destroying the device, ensure that it is not executing any work.
		vkDeviceWaitIdle(mDeviceHandle);

		vkDestroyDevice(mDeviceHandle, nullptr);
	}

	bool Device::isPhysicalDeviceSuitable(VkPhysicalDevice tPhysicalDevice) 
	{
		// In the future, we may want to construct a map of physical devices to "scores" so that we can
		// rank GPUs and choose a fallback device if the tests below fail.
		auto physicalDeviceProperties = getPhysicalDeviceProperties(tPhysicalDevice);
		auto physicalDeviceFeatures = getPhysicalDeviceFeatures(tPhysicalDevice);
		auto queueFamilyProperties = getPhysicalDeviceQueueFamilyProperties(tPhysicalDevice);

		bool foundSuitableQueueFamily = false;
		size_t queueFamilyIndex = 0;
		for (const auto &queueFamilyProperty : queueFamilyProperties)
		{
			// A queue can support one or more of the following:
			//	VK_QUEUE_GRAPHICS_BIT
			//	VK_QUEUE_COMPUTE_BIT
			//	VK_QUEUE_TRANSFER_BIT (copying buffer and image contents)
			//	VK_QUEUE_SPARSE_BINDING_BIT (memory binding operations used to update sparse resources)
			// For now, find a single queue family that supports graphics and compute operations and presentation.
			// In the future, it should be possible to use two or more different queue families for these operations.
			bool supportsAllQueueFlagBits = true;
			for (const auto &requiredQueueFlagBit : mRequiredQueueFlagBits)
			{
				if (!(queueFamilyProperty.queueFlags & requiredQueueFlagBit))
				{
					supportsAllQueueFlagBits = false;
					break;
				}
			}

			// Check if this queue family supports presentation and allows for the creation of at least one queue.
			VkBool32 supportsSurfacePresentation = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(tPhysicalDevice, queueFamilyIndex, mSurface->getHandle(), &supportsSurfacePresentation);

			if (supportsAllQueueFlagBits &&
				supportsSurfacePresentation &&
				queueFamilyProperty.queueCount > 0)
			{
				std::cout << "Found suitable queue family (index " << queueFamilyIndex << ") with " << queueFamilyProperty.queueCount << " possible queues\n";
				foundSuitableQueueFamily = true;
				break;
			}

			++queueFamilyIndex;
		}

		// Make sure that the physical device supports all of the device level extensions (i.e. swapchain creation).
		bool deviceExtensionsSupported = checkDeviceExtensionSupport(tPhysicalDevice);

		// Make sure that the physical device's swapchain support is adequate: only check this if the physical device supports swapchain creation.
		bool swapchainSupportIsAdequate = false;
		if (deviceExtensionsSupported)
		{
			// For now, a physical device's swapchain support is adequate if there is at least one supported image format and presentation mode.
			auto swapchainSupportDetails = getSwapchainSupportDetails(tPhysicalDevice);
			swapchainSupportIsAdequate = !swapchainSupportDetails.mFormats.empty() && !swapchainSupportDetails.mPresentModes.empty();
			std::cout << "This physical device's swapchain support is " << (swapchainSupportIsAdequate ? "adequate\n" : "inadequate\n");
		}

		if (foundSuitableQueueFamily &&
			deviceExtensionsSupported &&
			swapchainSupportIsAdequate &&
			physicalDeviceProperties.deviceType == mRequiredPhysicalDeviceType &&
			physicalDeviceFeatures.tessellationShader &&
			physicalDeviceFeatures.geometryShader)
		{
			std::cout << "Found suitable physical device:\n";
			std::cout << "\tdevice ID: " << physicalDeviceProperties.deviceID << "\n";
			std::cout << "\tdevice name: " << physicalDeviceProperties.deviceName << "\n";
			std::cout << "\tvendor ID: " << physicalDeviceProperties.vendorID << "\n";
			mQueueFamilyIndex = queueFamilyIndex;
			return true;
		}

		return false;
	}

	bool Device::checkDeviceExtensionSupport(VkPhysicalDevice tPhysicalDevice) const
	{
		auto deviceExtensionProperties = getDeviceExtensionProperties(tPhysicalDevice);

		// Make sure that all of the required device extensions are available.
		for (const auto &requiredDeviceExtensionName: mRequiredDeviceExtensions)
		{
			auto predicate = [&](const VkExtensionProperties &extensionProperty) { return strcmp(requiredDeviceExtensionName, extensionProperty.extensionName) == 0; };
			if (std::find_if(deviceExtensionProperties.begin(), deviceExtensionProperties.end(), predicate) == deviceExtensionProperties.end())
			{
				std::cout << "Required device extension " << requiredDeviceExtensionName << " is not supported by this physical device\n";
				return false;
			}
		}

		return true;
	}

	VkPhysicalDeviceProperties Device::getPhysicalDeviceProperties(VkPhysicalDevice tPhysicalDevice) const
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(tPhysicalDevice, &physicalDeviceProperties);

		return physicalDeviceProperties;
	}

	VkPhysicalDeviceFeatures Device::getPhysicalDeviceFeatures(VkPhysicalDevice tPhysicalDevice) const
	{
		VkPhysicalDeviceFeatures physicalDeviceFeatures;
		vkGetPhysicalDeviceFeatures(tPhysicalDevice, &physicalDeviceFeatures);

		return physicalDeviceFeatures;
	}

	std::vector<VkQueueFamilyProperties> Device::getPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice tPhysicalDevice) const
	{
		uint32_t physicalDeviceQueueFamilyPropertiesCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(tPhysicalDevice, &physicalDeviceQueueFamilyPropertiesCount, nullptr);

		std::vector<VkQueueFamilyProperties> physicalDeviceQueueFamilyProperties(physicalDeviceQueueFamilyPropertiesCount);
		vkGetPhysicalDeviceQueueFamilyProperties(tPhysicalDevice, &physicalDeviceQueueFamilyPropertiesCount, physicalDeviceQueueFamilyProperties.data());

		return physicalDeviceQueueFamilyProperties;
	}

	std::vector<VkExtensionProperties> Device::getDeviceExtensionProperties(VkPhysicalDevice tPhysicalDevice) const
	{
		uint32_t deviceExtensionPropertiesCount = 0;
		vkEnumerateDeviceExtensionProperties(tPhysicalDevice, nullptr, &deviceExtensionPropertiesCount, nullptr);

		std::vector<VkExtensionProperties> deviceExtensionProperties(deviceExtensionPropertiesCount);
		vkEnumerateDeviceExtensionProperties(tPhysicalDevice, nullptr, &deviceExtensionPropertiesCount, deviceExtensionProperties.data());

		return deviceExtensionProperties;
	}

	VkPhysicalDeviceMemoryProperties Device::getPhysicalDeviceMemoryProperties(VkPhysicalDevice tPhysicalDevice) const
	{
		VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
		vkGetPhysicalDeviceMemoryProperties(tPhysicalDevice, &physicalDeviceMemoryProperties);

		std::cout << "Enumerating this device's available memory types:\n";

		for (size_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
		{
			VkMemoryType memoryType = physicalDeviceMemoryProperties.memoryTypes[i];
			std::cout << "Found an available memory type with heap index: " << memoryType.heapIndex << "\n";

			if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			{
				std::cout << "\tThis is device local memory (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)\n";
			}
			if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
			{
				std::cout << "\tThis is host visible memory (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)\n";
			}
			if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			{
				std::cout << "\tThis is host coherent memory (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)\n";
			}
			if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
			{
				std::cout << "\tThis is host cached memory (VK_MEMORY_PROPERTY_HOST_CACHED_BIT)\n";
			}
			if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
			{
				std::cout << "\tThis is lazily allocated memory (VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)\n";
			}
		}

		std::cout << "Enumerating this device's available memory heaps:\n";

		for (size_t i = 0; i < physicalDeviceMemoryProperties.memoryHeapCount; ++i)
		{
			VkMemoryHeap memoryHeap = physicalDeviceMemoryProperties.memoryHeaps[i];

			std::cout << "Found an available memory heap with size: " << memoryHeap.size << " (bytes)\n";

			// VkMemoryHeapFlags will always be VK_MEMORY_HEAP_DEVICE_LOCAL_BIT
		}

		return physicalDeviceMemoryProperties;
	}

	SwapchainSupportDetails Device::getSwapchainSupportDetails(VkPhysicalDevice tPhysicalDevice) const
	{
		SwapchainSupportDetails details;

		// General surface capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(tPhysicalDevice, mSurface->getHandle(), &details.mCapabilities);

		// Surface formats
		uint32_t formatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(tPhysicalDevice, mSurface->getHandle(), &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.mFormats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(tPhysicalDevice, mSurface->getHandle(), &formatCount, details.mFormats.data());
		}

		// Surface presentation modes
		uint32_t presentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(tPhysicalDevice, mSurface->getHandle(), &presentModeCount, nullptr);

		if (presentModeCount != 0) 
		{
			details.mPresentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(tPhysicalDevice, mSurface->getHandle(), &presentModeCount, details.mPresentModes.data());
		}

		return details;
	}

} // namespace vk