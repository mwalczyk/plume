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

//! helper function for reading the contents of a file
static std::vector<char> readFile(const std::string &tFileName)
{
	// start reading at the end of the file to determine file size
	std::ifstream file(tFileName, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file");
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> fileContents(fileSize);

	file.seekg(0);
	file.read(fileContents.data(), fileSize);

	file.close();

	std::cout << "successfully read " << fileSize << " bytes from file: " << tFileName << "\n";

	return fileContents;
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
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createCommandBuffers();
	createSemaphores();
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
	auto selectedExtent = selectSwapchainExtent(swapchainSupportDetails.mCapabilities);

	// if the maxImageCount field is 0, this indicates that there is no limit (besides memory requirements) to the number of images in the swapchain
	uint32_t imageCount = swapchainSupportDetails.mCapabilities.minImageCount + 1;
	if (swapchainSupportDetails.mCapabilities.maxImageCount > 0 && imageCount > swapchainSupportDetails.mCapabilities.maxImageCount)
	{
		imageCount = swapchainSupportDetails.mCapabilities.maxImageCount;
	}
	std::cout << "creating a swapchain with " << imageCount << " images\n";

	// for now, assume that the graphics and presentation queues are the same - this is indicated by the VK_SHARING_MODE_EXCLUSIVE flag
	// in the future, we will need to account for the fact that these two operations may be a part of different queue families
	VkSwapchainCreateInfoKHR swapchainCreateInfo;
	swapchainCreateInfo.clipped = VK_TRUE;										// ignore pixels that are obscured by other windows
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;		// this window should not blend with other windows
	swapchainCreateInfo.flags = 0;
	swapchainCreateInfo.imageArrayLayers = 1;									// only greater than 1 when performing stereo rendering
	swapchainCreateInfo.imageColorSpace = selectedSurfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = selectedExtent;
	swapchainCreateInfo.imageFormat = selectedSurfaceFormat.format;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;			// this swapchain is only accessed by one queue family (see notes above)
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;							
	swapchainCreateInfo.pNext = nullptr;
	swapchainCreateInfo.pQueueFamilyIndices = nullptr;							// if the sharing mode is exlusive, we don't need to specify this 
	swapchainCreateInfo.presentMode = selectedPresentMode;
	swapchainCreateInfo.preTransform = swapchainSupportDetails.mCapabilities.currentTransform;
	swapchainCreateInfo.queueFamilyIndexCount = 0;								// if the sharing mode is exlusive, we don't need to specify this 
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = mSurface;

	if (vkCreateSwapchainKHR(mLogicalDevice, &swapchainCreateInfo, nullptr, &mSwapchain) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swapchain");
	}

	std::cout << "successfully created swapchain\n";

	// note that the Vulkan implementation may create more swapchain images than requested above - this is why we query the number of images again
	vkGetSwapchainImagesKHR(mLogicalDevice, mSwapchain, &imageCount, nullptr);
	mSwapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(mLogicalDevice, mSwapchain, &imageCount, mSwapchainImages.data());
	std::cout << "retrieved " << imageCount << " images from the swapchain\n";
	
	// store the image format and extent for later use
	mSwapchainImageFormat = selectedSurfaceFormat.format;
	mSwapchainImageExtent = selectedExtent;
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
		std::cout << "VkSurfaceCapabilitiesKHR currentExtent member isn't set to the maximum value of uint32_t - defaulting to:\n";
		std::cout << "\twidth: " << tSurfaceCapabilities.currentExtent.width << ", height: " << tSurfaceCapabilities.currentExtent.height << "\n";
		return tSurfaceCapabilities.currentExtent;
	}
	else
	{
		VkExtent2D actualExtent = { mWindowWidth, mWindowHeight };
		actualExtent.width = std::max(tSurfaceCapabilities.minImageExtent.width, std::min(tSurfaceCapabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(tSurfaceCapabilities.minImageExtent.height, std::min(tSurfaceCapabilities.maxImageExtent.height, actualExtent.height));
		std::cout << "VkSurfaceCapabilitiesKHR currentExtent member is set to the maximum value of uint32_t - setting to:\n";
		std::cout << "\twidth: " << actualExtent.width << ", height: " << actualExtent.height << "\n";
		return actualExtent;
	}
}

void App::createImageViews()
{
	mSwapchainImageViews.resize(mSwapchainImages.size());

	for (size_t i = 0; i < mSwapchainImageViews.size(); ++i)
	{
		VkImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;				// do not swizzle any of the color channels
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.format = mSwapchainImageFormat;
		imageViewCreateInfo.image = mSwapchainImages[i];
		imageViewCreateInfo.pNext = nullptr;
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// describes the image's purpose
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;						// describes which part of the image we will access
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;							// treat the image as a standard 2D texture
	
		if (vkCreateImageView(mLogicalDevice, &imageViewCreateInfo, nullptr, &mSwapchainImageViews[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create one or more image views for the swapchain");
		}
	}

	std::cout << "successfully created " << mSwapchainImageViews.size() << " image views for the swapchain\n";
}

void App::createRenderPass()
{
	// for now, we simply have a single color attachment, represented by one of the images from the swapchain
	VkAttachmentDescription attachmentDescription = {};
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	attachmentDescription.flags = 0;
	attachmentDescription.format = mSwapchainImageFormat;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;		// clear the existing contents of the attachment prior to rendering
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;			// no multisampling
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;	// rendered contents will be stored in memory for later reads

	// create a reference to the attachment described above
	VkAttachmentReference attachmentReference = {};
	attachmentReference.attachment = 0;
	attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// a single render pass can consist of multiple subpasses (subsequent rendering operations)
	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &attachmentReference;

	// create a subpass dependency 
	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// create a render pass with the information above
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.pAttachments = &attachmentDescription;
	renderPassCreateInfo.pDependencies = &subpassDependency;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.subpassCount = 1;

	if (vkCreateRenderPass(mLogicalDevice, &renderPassCreateInfo, nullptr, &mRenderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass");
	}

	std::cout << "successfully created render pass\n";
}

void App::createGraphicsPipeline()
{
	// for now, use absolute paths
	auto vertShaderSrc = readFile("C:/Users/michael.walczyk/Documents/Visual Studio 2015/Projects/VulkanToolkit/assets/shaders/vert.spv");
	auto fragShaderSrc = readFile("C:/Users/michael.walczyk/Documents/Visual Studio 2015/Projects/VulkanToolkit/assets/shaders/frag.spv");

	// shader module objects are only required during the pipeline creation process
	VkShaderModule vertShaderModule{ VK_NULL_HANDLE };
	VkShaderModule fragShaderModule{ VK_NULL_HANDLE };
	createShaderModule(vertShaderSrc, &vertShaderModule);
	createShaderModule(fragShaderSrc, &fragShaderModule);
	
	// assign shader modules to specific shader stages
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.flags = 0;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.pNext = nullptr;
	vertShaderStageInfo.pSpecializationInfo = nullptr;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.flags = 0;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";
	fragShaderStageInfo.pNext = nullptr;
	fragShaderStageInfo.pSpecializationInfo = nullptr;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

	// group the create info structures together
	VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[] = { vertShaderStageInfo, fragShaderStageInfo };
	

	// describe the format of the vertex data that will be passed to the vertex shader
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.flags = 0;
	vertexInputStateCreateInfo.pNext = nullptr;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;

	// describe the type of geometry that will be drawn and if primitive restart should be enabled
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.flags = 0;
	inputAssemblyStateCreateInfo.pNext = nullptr;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// set up a viewport
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = static_cast<float>(mSwapchainImageExtent.width);
	viewport.height = static_cast<float>(mSwapchainImageExtent.height);
	viewport.minDepth = 0.0f;	// the range of depth values that will be used by the depth buffer
	viewport.maxDepth = 1.0f;

	// set up a fullscreen scissor rectangle
	VkRect2D scissor = {};
	scissor.extent = mSwapchainImageExtent;
	scissor.offset = { 0, 0 };

	// combine the viewport and scissor settings into a viewport state structure
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.flags = 0;
	viewportStateCreateInfo.pNext = nullptr;
	viewportStateCreateInfo.pScissors = &scissor;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;

	// configure the rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;	// turn on backface culling
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.flags = 0;
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationStateCreateInfo.lineWidth = 1.0f;
	rasterizationStateCreateInfo.pNext = nullptr;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

	// configure multisampling (anti-aliasing): for now, disable this feature
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
	multisampleStateCreateInfo.flags = 0;
	multisampleStateCreateInfo.minSampleShading = 1.0f;
	multisampleStateCreateInfo.pNext = nullptr;
	multisampleStateCreateInfo.pSampleMask = nullptr;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

	// for now, we are not using depth and stencil tests

	// configure color blending - this determines how new fragments are composited with colors that are already in the framebuffer
	// note that there are two structures necessary for setting up color blending
	// each attached framebuffer has a VkPipelineColorBlendAttachmentState, while a single VkPipelineColorBlendStateCreateInfo structure contains the global color blending settings
	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.blendEnable = VK_FALSE;		// blending is currently disabled, so the rest of these parameters are optional
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;

	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
	colorBlendStateCreateInfo.blendConstants[3] = 0.0f;
	colorBlendStateCreateInfo.flags = 0;
	colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;		// if true, the logic op described here will override the blend modes for every attached framebuffer
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
	colorBlendStateCreateInfo.pNext = nullptr;
	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

	// a limited amount of the pipeline state can be changed without recreating the entire pipeline - see VkPipelineDynamicStateCreateInfo
	
	// for now, create an empty pipeline layout 
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
	pipelineLayoutCreateInfo.pSetLayouts = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	if (vkCreatePipelineLayout(mLogicalDevice, &pipelineLayoutCreateInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout");
	}

	std::cout << "successfully created pipeline layout\n";

	// aggregate all of the structures above to create a graphics pipeline
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	// we aren't referencing an existing pipeline
	graphicsPipelineCreateInfo.basePipelineIndex = -1;
	graphicsPipelineCreateInfo.flags = 0;
	graphicsPipelineCreateInfo.layout = mPipelineLayout;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
	graphicsPipelineCreateInfo.pDynamicState = nullptr;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pNext = nullptr;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pStages = pipelineShaderStageCreateInfos;
	graphicsPipelineCreateInfo.pTessellationState = nullptr;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.renderPass = mRenderPass;
	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.subpass = 0;

	if (vkCreateGraphicsPipelines(mLogicalDevice, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &mPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline");
	}

	std::cout << "successfully created pipeline\n";

	vkDestroyShaderModule(mLogicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(mLogicalDevice, fragShaderModule, nullptr);
}

void App::createShaderModule(const std::vector<char> &tSrc, VkShaderModule *tShaderModule)
{
	VkShaderModuleCreateInfo shaderModuleCreateInfo;
	shaderModuleCreateInfo.codeSize = tSrc.size();
	shaderModuleCreateInfo.flags = 0;
	shaderModuleCreateInfo.pCode = (uint32_t*)tSrc.data();
	shaderModuleCreateInfo.pNext = nullptr;
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	if (vkCreateShaderModule(mLogicalDevice, &shaderModuleCreateInfo, nullptr, tShaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module");
	}

	std::cout << "successfully created shader module\n";
}

void App::createFramebuffers()
{
	mSwapchainFramebuffers.resize(mSwapchainImages.size());

	for (size_t i = 0; i < mSwapchainImages.size(); ++i)	
	{
		VkImageView attachments[] = { mSwapchainImageViews[i] };

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.height = mSwapchainImageExtent.height;
		framebufferCreateInfo.layers = 1;
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.pNext = nullptr;
		framebufferCreateInfo.renderPass = mRenderPass;	// the render pass that this framebuffer needs to be compatible with
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.width = mSwapchainImageExtent.width;

		if (vkCreateFramebuffer(mLogicalDevice, &framebufferCreateInfo, nullptr, &mSwapchainFramebuffers[i]))
		{
			throw std::runtime_error("failed to create one or more of the swapchain framebuffers");
		}
	}

	std::cout << "successfully created " << mSwapchainFramebuffers.size() << " framebuffers for the swapchain\n";
}

void App::createCommandPool()
{
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.flags = 0; // possible flags are VK_COMMAND_POOL_CREATE_TRANSIENT_BIT and VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.queueFamilyIndex = mQueueFamilyIndex;
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

	if (vkCreateCommandPool(mLogicalDevice, &commandPoolCreateInfo, nullptr, &mCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool");
	}

	std::cout << "successfully created command pool\n";
}

void App::createCommandBuffers()
{
	// allocate and record the commands for each swapchain image
	mCommandBuffers.resize(mSwapchainFramebuffers.size());

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = mCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = (uint32_t)mCommandBuffers.size();

	if (vkAllocateCommandBuffers(mLogicalDevice, &commandBufferAllocateInfo, mCommandBuffers.data()) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to allocate command buffers");
	}

	for (size_t i = 0; i < mCommandBuffers.size(); ++i)
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo = {};
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		commandBufferBeginInfo.pInheritanceInfo = nullptr;
		commandBufferBeginInfo.pNext = nullptr;
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		// start recording into this command buffer
		vkBeginCommandBuffer(mCommandBuffers[i], &commandBufferBeginInfo);

		VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.framebuffer = mSwapchainFramebuffers[i];
		renderPassBeginInfo.pClearValues = &clearValue;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderArea.extent = mSwapchainImageExtent;
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderPass = mRenderPass;
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

		// begin the render pass
		vkCmdBeginRenderPass(mCommandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// bind the graphics pipeline
		vkCmdBindPipeline(mCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

		// draw
		vkCmdDraw(mCommandBuffers[i], 3, 1, 0, 0);

		// stop recording into this command buffer
		vkCmdEndRenderPass(mCommandBuffers[i]);

		if (vkEndCommandBuffer(mCommandBuffers[i]) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to record command buffer");
		}
	}

	std::cout << "successfully recorded " << mCommandBuffers.size() << " command buffers\n";
}

void App::createSemaphores()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.flags = 0;
	semaphoreCreateInfo.pNext = nullptr;
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(mLogicalDevice, &semaphoreCreateInfo, nullptr, &mImageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(mLogicalDevice, &semaphoreCreateInfo, nullptr, &mRenderFinishedSemaphore) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create semaphores");
	}
	
	std::cout << "successfully created semaphores\n";
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
	// get the index of the next available image
	uint32_t imageIndex{ 0 };
	vkAcquireNextImageKHR(mLogicalDevice, mSwapchain, std::numeric_limits<uint64_t>::max(), mImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { mImageAvailableSemaphore };
	VkSemaphore signalSemaphores[] = { mRenderFinishedSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;		// wait for the image to be acquired from the swapchain
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mCommandBuffers[imageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;	// signal that rendering has finished

	if (vkQueueSubmit(mQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to submit draw command buffer");
	}

	// submit the result back to the swapchain for presentation
	VkSwapchainKHR swapchains[] = { mSwapchain };
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;		// wait for rendering to finish before attempting to present
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	// present
	vkQueuePresentKHR(mQueue, &presentInfo);
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