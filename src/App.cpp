#include "App.h"

//! proxy function for creating a debug callback object
VkResult createDebugReportCallbackEXT(VkInstance tInstance,
	const VkDebugReportCallbackCreateInfoEXT* tCreateInfo,
	const VkAllocationCallbacks* tAllocator,
	VkDebugReportCallbackEXT* tCallback)
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

//! proxy function for deleting a debug callback object
void destroyDebugReportCallbackEXT(VkInstance tInstance,
	VkDebugReportCallbackEXT tCallback,
	const VkAllocationCallbacks* tAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(tInstance, "vkDestroyDebugReportCallbackEXT");

	if (func != nullptr)
	{
		func(tInstance, tCallback, tAllocator);
	}
}

//! create Vulkan objects
void App::initializeRenderer()
{
	createInstance();
	setupDebugCallback();
	createSurface();
	createPhysicalDevice();
	createLogicalDevice();
	createSwapchain();
}

void App::initializeWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	std::cout << "glfw version: " << glfwGetVersionString() << "\n";
	mWindowHandle = glfwCreateWindow(mWindowWidth, mWindowHeight, mApplicationName.c_str(), nullptr, nullptr);
}

void App::createInstance()
{
	// handle validation layers
	if (!checkValidationLayerSupport())
	{
		throw std::runtime_error("one or more of the requested validation layers are not supported on this platform");
	}

	VkApplicationInfo applicationInfo;
	applicationInfo.apiVersion = VK_API_VERSION_1_0;
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.pApplicationName = mApplicationName.c_str();
	applicationInfo.pEngineName = "engine";
	applicationInfo.pNext = nullptr;
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

	// get the extensions required by glfw: VK_KHR_surface and VK_KHR_win32_surface
	if (mWindowMode == WindowMode::TOOLKIT_GLFW_WINDOW)
	{
		uint32_t glfwRequiredExtensionCount{ 0 };
		const char** glfwRequiredExtensionNames;
		glfwRequiredExtensionNames = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);
		for (size_t i = 0; i < glfwRequiredExtensionCount; ++i)
		{
			mRequiredExtensions.push_back(glfwRequiredExtensionNames[i]);
		}
	}

	VkInstanceCreateInfo instanceCreateInfo;
	instanceCreateInfo.enabledExtensionCount = mRequiredExtensions.size();
	instanceCreateInfo.enabledLayerCount = mRequiredLayers.size();
	instanceCreateInfo.flags = 0;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.ppEnabledExtensionNames = mRequiredExtensions.data();
	instanceCreateInfo.ppEnabledLayerNames = mRequiredLayers.data();
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

	if (vkCreateInstance(&instanceCreateInfo, nullptr, &mInstance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create a valid Vulkan instance");
	}
}

//! check if all requested validation layers are supported on this device
bool App::checkValidationLayerSupport()
{
	uint32_t layerCount{ 0 };
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const auto& layerName : mRequiredLayers)
	{
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers)
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

//! retrieve a list of all available instance level extensions
std::vector<VkExtensionProperties> App::getExtensionProperties() const
{
	uint32_t extensionPropertiesCount;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertiesCount, nullptr);

	std::vector<VkExtensionProperties> extensionProperties(extensionPropertiesCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertiesCount, extensionProperties.data());
	
	std::cout << "this Vulkan instance supports " << extensionPropertiesCount << " extensions:\n";

	for (const auto &extensionProperty: extensionProperties)
	{
		std::cout << "\textension name: " << extensionProperty.extensionName << ", spec version: " << extensionProperty.specVersion << "\n";
	}

	return extensionProperties;
}

//! setup the callback function for validation layers
void App::setupDebugCallback()
{
	VkDebugReportCallbackCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	createInfo.pfnCallback = debugCallback;

	if (createDebugReportCallbackEXT(mInstance, &createInfo, nullptr, &mDebugCallback) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to setup the debug callback");
	}
}

void App::createSurface()
{
	if (glfwCreateWindowSurface(mInstance, mWindowHandle, nullptr, &mSurface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create surface");
	}

	std::cout << "successfully created surface\n";
}

void App::createPhysicalDevice()
{
	uint32_t physicalDeviceCount{ 0 };
	vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, nullptr);

	if (physicalDeviceCount == 0)
	{
		throw std::runtime_error("failed to find any connected physical devices");
	}

	std::cout << "there are " << physicalDeviceCount << " physical devices connected\n";

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, physicalDevices.data());

	for (const auto& physicalDevice : physicalDevices)
	{
		if (isPhysicalDeviceSuitable(physicalDevice))
		{
			mPhysicalDevice = physicalDevice;
		}
	}

	if (mPhysicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find a suitable physical device");
	}
}

//! after enumerating all physical devices on the system, pick one that is suitable for graphics and compute operations
bool App::isPhysicalDeviceSuitable(VkPhysicalDevice tPhysicalDevice)
{
	auto physicalDeviceProperties = getPhysicalDeviceProperties(tPhysicalDevice);
	auto physicalDeviceFeatures = getPhysicalDeviceFeatures(tPhysicalDevice);

	// in the future, we may want to construct a map of physical devices to "scores" so that we can
	// rank GPUs and choose a fallback device if the tests below fail

	// make sure that the physical device has a queue family that supports both graphics and compute operations
	auto queueFamilyProperties = getPhysicalDeviceQueueFamilyProperties(tPhysicalDevice);
	bool foundSuitableQueueFamily{ false };
	size_t queueFamilyIndex{ 0 };

	// check the capabilites of each queue family that is supported on this device
	for (const auto &queueFamilyProperty : queueFamilyProperties)
	{
		// a queue can support one or more of the following:
		// VK_QUEUE_GRAPHICS_BIT
		// VK_QUEUE_COMPUTE_BIT
		// VK_QUEUE_TRANSFER_BIT (copying buffer and image contents)
		// VK_QUEUE_SPARSE_BINDING_BIT (memory binding operations used to update sparse resources)
		
		// for now, find a single queue family that supports graphics and compute operations and presentation
		// in the future, it should be possible to use two or more different queue families for these operations
		
		bool supportsAllQueueFlagBits{ true };
		for (const auto &requiredQueueFlagBit : mRequiredQueueFlagBits)
		{
			if (!(queueFamilyProperty.queueFlags & requiredQueueFlagBit))
			{
				supportsAllQueueFlagBits = false;
				break;
			}
		}
		
		// check if this queue family supports presentation
		VkBool32 presentSupport{ false };
		vkGetPhysicalDeviceSurfaceSupportKHR(tPhysicalDevice, queueFamilyIndex, mSurface, &presentSupport);

		if (supportsAllQueueFlagBits &&
			presentSupport &&
			queueFamilyProperty.queueCount > 0)
		{
			std::cout << "found suitable queue family (index " << queueFamilyIndex << ") with " << queueFamilyProperty.queueCount << " possible queues\n";
			foundSuitableQueueFamily = true;
			break;
		}

		++queueFamilyIndex;
	}

	// make sure that the physical device supports all of the device level extensions (i.e. swapchain creation)
	bool deviceExtensionsSupported = checkDeviceExtensionSupport(tPhysicalDevice);

	// make sure that the physical device's swapchain support is adequate: only check this if the physical device supports swapchain creation
	bool swapchainSupportIsAdequate{ false };
	if (deviceExtensionsSupported)
	{
		// for now, a physical device's swapchain support is adequate if there is at least one supported image format and presentation mode
		auto swapchainSupportDetails = getSwapchainSupportDetails(tPhysicalDevice);
		swapchainSupportIsAdequate = !swapchainSupportDetails.mFormats.empty() && !swapchainSupportDetails.mPresentModes.empty();
		std::cout << "this physical device's swapchain support is " << (swapchainSupportIsAdequate ? "adequate\n" : "inadequate\n");
	}

	// make sure that the physical device is a discrete GPU and supports both tessellation and geometry shaders
	if (foundSuitableQueueFamily &&
		deviceExtensionsSupported &&
		swapchainSupportIsAdequate &&
		physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		physicalDeviceFeatures.tessellationShader &&
		physicalDeviceFeatures.geometryShader)
	{
		std::cout << "found suitable physical device:\n";
		std::cout << "\tdevice ID: " << physicalDeviceProperties.deviceID << "\n";
		std::cout << "\tdevice name: " << physicalDeviceProperties.deviceName << "\n";
		std::cout << "\tvendor ID: " << physicalDeviceProperties.vendorID << "\n";
		mQueueFamilyIndex = queueFamilyIndex;
		return true;
	}

	return false;
}

bool App::checkDeviceExtensionSupport(VkPhysicalDevice tPhysicalDevice)
{
	auto deviceExtensionProperties = getPhysicalDeviceExtensionProperties(tPhysicalDevice);

	// for now, print all of the available device extension names
	std::cout << "this device supports " << deviceExtensionProperties.size() << " extensions:\n";
	for (const auto &deviceExtensionProperty : deviceExtensionProperties)
	{
		std::cout << "\textension name: " << deviceExtensionProperty.extensionName << ", spec version: " << deviceExtensionProperty.specVersion << "\n";
	}

	// make sure that all of the required device extensions are available
	for (const auto &requiredDeviceExtensionName : mRequiredDeviceExtensions)
	{
		std::cout << "checking support for device extension: " << requiredDeviceExtensionName << "\n";
		auto predicate = [&](const VkExtensionProperties &extensionProperty) { return strcmp(requiredDeviceExtensionName, extensionProperty.extensionName) == 0; };
		if (std::find_if(deviceExtensionProperties.begin(), deviceExtensionProperties.end(), predicate) == deviceExtensionProperties.end())
		{
			std::cout << "required device extension " << requiredDeviceExtensionName << " is not supported by this physical device\n";
			return false;
		}
	}

	std::cout << "all of the required device extensions are supported by this physical device\n";

	return true;
}

//! retrieve some properties about this physical device's swapchain
SwapchainSupportDetails App::getSwapchainSupportDetails(VkPhysicalDevice tPhysicalDevice) const
{
	// all of the support querying functions take a VkPhysicalDevice and VkSurfaceKHR as parameters
	SwapchainSupportDetails details;

	// general surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(tPhysicalDevice, mSurface, &details.mCapabilities);

	// surface formats
	uint32_t formatCount{ 0 };
	vkGetPhysicalDeviceSurfaceFormatsKHR(tPhysicalDevice, mSurface, &formatCount, nullptr);
	if (formatCount != 0)
	{
		details.mFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(tPhysicalDevice, mSurface, &formatCount, details.mFormats.data());
	}

	// surface presentation modes
	uint32_t presentModeCount{ 0 };
	vkGetPhysicalDeviceSurfacePresentModesKHR(tPhysicalDevice, mSurface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.mPresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(tPhysicalDevice, mSurface, &presentModeCount, details.mPresentModes.data());
	}

	return details;
}

VkPhysicalDeviceProperties App::getPhysicalDeviceProperties(VkPhysicalDevice tPhysicalDevice) const
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(tPhysicalDevice, &physicalDeviceProperties);

	return physicalDeviceProperties;
}

VkPhysicalDeviceFeatures App::getPhysicalDeviceFeatures(VkPhysicalDevice tPhysicalDevice) const
{
	VkPhysicalDeviceFeatures physicalDeviceFeatures;
	vkGetPhysicalDeviceFeatures(tPhysicalDevice, &physicalDeviceFeatures);

	return physicalDeviceFeatures;
}

std::vector<VkQueueFamilyProperties> App::getPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice tPhysicalDevice) const
{
	uint32_t queueFamilyPropertyCount{ 0 };
	vkGetPhysicalDeviceQueueFamilyProperties(tPhysicalDevice, &queueFamilyPropertyCount, nullptr);

	std::cout << "this physical device supports " << queueFamilyPropertyCount << " queue families\n";

	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(tPhysicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

	return queueFamilyProperties;
}

std::vector<VkExtensionProperties> App::getPhysicalDeviceExtensionProperties(VkPhysicalDevice tPhysicalDevice) const
{
	uint32_t deviceExtensionPropertiesCount{ 0 };
	vkEnumerateDeviceExtensionProperties(tPhysicalDevice, nullptr, &deviceExtensionPropertiesCount, nullptr);

	std::vector<VkExtensionProperties> deviceExtensionProperties(deviceExtensionPropertiesCount);
	vkEnumerateDeviceExtensionProperties(tPhysicalDevice, nullptr, &deviceExtensionPropertiesCount, deviceExtensionProperties.data());

	return deviceExtensionProperties;
}

VkPhysicalDeviceMemoryProperties App::getPhysicalDeviceMemoryProperties(VkPhysicalDevice tPhysicalDevice) const
{
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(tPhysicalDevice, &physicalDeviceMemoryProperties);

	std::cout << "enumerating this device's available memory types:\n";

	for (size_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
	{
		VkMemoryType memoryType = physicalDeviceMemoryProperties.memoryTypes[i];
		std::cout << "found an available memory type with heap index: " << memoryType.heapIndex << "\n";

		if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		{
			std::cout << "\tthis is device local memory (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)\n";
		}
		if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		{
			std::cout << "\tthis is host visible memory (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)\n";
		}
		if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		{
			std::cout << "\tthis is host coherent memory (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)\n";
		}
		if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
		{
			std::cout << "\tthis is host cached memory (VK_MEMORY_PROPERTY_HOST_CACHED_BIT)\n";
		}
		if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
		{
			std::cout << "\tthis is lazily allocated memory (VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)\n";
		}
	}

	std::cout << "enumerating this device's available memory heaps:\n";

	for (size_t i = 0; i < physicalDeviceMemoryProperties.memoryHeapCount; ++i)
	{
		VkMemoryHeap memoryHeap = physicalDeviceMemoryProperties.memoryHeaps[i];

		std::cout << "found an available memory heap with size: " << memoryHeap.size << " (bytes)\n";

		// VkMemoryHeapFlags will always be VK_MEMORY_HEAP_DEVICE_LOCAL_BIT
	}

	return physicalDeviceMemoryProperties;
}

void App::createLogicalDevice()
{
	// enumerate the features supported by the physical device but ensure that we always have access to tessellation and geometry shaders
	auto supportedFeatures = getPhysicalDeviceFeatures(mPhysicalDevice);
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

	if (vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mLogicalDevice) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create a logical device");
	}

	// grab a handle to the queue
	vkGetDeviceQueue(mLogicalDevice, mQueueFamilyIndex, 0, &mQueue);

	std::cout << "sucessfully created a logical device\n";
}

void App::createSwapchain()
{
	auto swapchainSupportDetails = getSwapchainSupportDetails(mPhysicalDevice);

	// from the structure above, determine an optimal surface format, presentation mode, and size for the swapchain
	auto selectedSurfaceFormat = selectSwapchainSurfaceFormat(swapchainSupportDetails.mFormats);
	auto selectedPresentMode = selectSwapchainPresentMode(swapchainSupportDetails.mPresentModes);
}

VkSurfaceFormatKHR App::selectSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &tSurfaceFormats) const
{
	// if there is only one VkSurfaceFormatKHR entry with format VK_FORMAT_UNDEFINED, this means that the surface has no preferred format
	if (tSurfaceFormats.size() == 1 && 
		tSurfaceFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		std::cout << "no preferred surface format - defaulting to VK_FORMAT_B8G8R8A8_UNORM and VK_COLOR_SPACE_SRGB_NONLINEAR_KHR\n";
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	// otherwise, there is a preferred format - iterate through and see if the above combination is available
	for (const auto &surfaceFormat : tSurfaceFormats)
	{
		if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && 
			surfaceFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
		{
			std::cout << "found the preferrd surface format - VK_FORMAT_B8G8R8A8_UNORM and VK_COLOR_SPACE_SRGB_NONLINEAR_KHR\n";
			return surfaceFormat;
		}
	}

	// at this point, we could start ranking the available formats and determine which one is "best"
	// for now, return the first available format, since our preferred format was not available
	std::cout << "could not find the preferred surface format - defaulting to the first available format\n";
	return tSurfaceFormats[0];
}

VkPresentModeKHR App::selectSwapchainPresentMode(const std::vector<VkPresentModeKHR> &tPresentModes) const
{
	// the swapchain can use one of the following modes for presentation:
	// VK_PRESENT_MODE_IMMEDIATE_KHR
	// VK_PRESENT_MODE_FIFO_KHR (the only mode guaranteed to be available)
	// VK_PRESENT_MODE_FIFO_RELAXED_KHR
	// VK_PRESENT_MODE_MAILBOX_KHR

	for (const auto& presentMode : tPresentModes)
	{
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			std::cout << "found VK_PRESENT_MODE_MAILBOX_KHR presentation mode\n";
			return presentMode;
		}
	}

	std::cout << "presentation mode VK_PRESENT_MODE_MAILBOX_KHR is not available - defaulting to VK_PRESENT_MODE_FIFO_KHR\n";
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D App::selectSwapchainExtent(const VkSurfaceCapabilitiesKHR &tSurfaceCapabilities) const
{
	if (tSurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return tSurfaceCapabilities.currentExtent;
	}
	else
	{
		VkExtent2D actualExtent = { mWindowWidth, mWindowHeight };
		
		actualExtent.width = std::max(tSurfaceCapabilities.minImageExtent.width, std::min(tSurfaceCapabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(tSurfaceCapabilities.minImageExtent.height, std::min(tSurfaceCapabilities.maxImageExtent.height, actualExtent.height));
	}

	return VkExtent2D{};
}

void App::setup()
{
	initializeWindow();
	initializeRenderer();
}

void App::update()
{

}

void App::draw()
{

}

void App::run()
{
	while (!glfwWindowShouldClose(mWindowHandle))
	{
		glfwPollEvents();
		update();
		draw();
	}
}