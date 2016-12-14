#include <iostream>
#include <vector>
#include <array>
#include <string>

#include "vulkan.h"

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

class App
{
public:
	//! pass an instance of this struct to the App constructor to enable extensions, validation layers, etc.
	struct Options
	{
		std::string mApplicationName;
		VkPhysicalDeviceFeatures mRequiredFeatures;
		bool mHeadless;

		//! default ctor
		Options() :
			mApplicationName("vulkan application"),
			mHeadless(false)
		{
			mRequiredFeatures.tessellationShader = VK_TRUE;
			mRequiredFeatures.geometryShader = VK_TRUE;
		}
		
		Options& applicationName(const std::string &tApplicationName) { mApplicationName = tApplicationName; return *this; }
		Options& requiredFeatures(VkPhysicalDeviceFeatures tRequiredFeatures) { mRequiredFeatures = tRequiredFeatures; return *this; }
	
	};

	App(const std::string &tApplicationName, VkPhysicalDeviceFeatures tRequiredFeatures) :
		mApplicationName{ tApplicationName }
	{
		mRequiredFeatures = tRequiredFeatures; // error if placed in initializer list?
		initializeRenderer();
		initializeWindow();
	};

private:
	void initializeRenderer();
	void initializeWindow();

	// 1.
	void createInstance();
	bool checkValidationLayerSupport();

	// 2.
	void setupDebugCallback();

	// 3.
	void createPhysicalDevice();
	bool isPhysicalDeviceSuitable(VkPhysicalDevice tPhysicalDevice);
	VkPhysicalDeviceProperties getPhysicalDeviceProperties(VkPhysicalDevice tPhysicalDevice);
	VkPhysicalDeviceFeatures getPhysicalDeviceFeatures(VkPhysicalDevice tPhysicalDevice);
	std::vector<VkQueueFamilyProperties> getPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice tPhysicalDevice);
	void getPhysicalDeviceMemoryProperties(VkPhysicalDevice tPhysicalDevice);

	// 4.
	void createLogicalDevice();
	
	// app requirements
	std::vector<const char*> mRequiredLayers = { "VK_LAYER_LUNARG_standard_validation" };					// what Vulkan validation layers does this app need to support?
	std::vector<const char*> mRequiredExtensions = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME };					// what Vulkan extensions does this app need to support?
	VkPhysicalDeviceFeatures mRequiredFeatures = {};														// what Vulkan features does this app need to support?
	std::vector<VkQueueFlagBits> mRequiredQueueFlagBits = { VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT };	// what types of operations does this app need to support?

	// general app properties
	std::string mApplicationName;
	uint32_t mWindowWidth = { 640 };
	uint32_t mWindowHeight = { 480 };

	VkDebugReportCallbackEXT mDebugCallback;

	// handles to Vulkan objects
	VkInstance mInstance{ VK_NULL_HANDLE };
	VkPhysicalDevice mPhysicalDevice{ VK_NULL_HANDLE };
	VkDevice mLogicalDevice{ VK_NULL_HANDLE };

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData
	)
	{	
		std::cerr << "validation layer: " << msg << std::endl;
		return VK_FALSE;
	}
};

void App::initializeRenderer()
{
	createInstance();
	setupDebugCallback();
	createPhysicalDevice();
	createLogicalDevice();
}

void App::initializeWindow()
{
	// TODO: glfw integration
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

//! check if all request validation layers are supported on this device
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

void App::createPhysicalDevice()
{
	// how many physical devices are connected?
	uint32_t physicalDeviceCount{ 0 };
	vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, nullptr);

	// make sure there is at least one physical device connected
	if (physicalDeviceCount == 0)
	{
		throw std::runtime_error("failed to find any connected physical devices");
	}

	std::cout << "there are " << physicalDeviceCount << " physical devices connected\n";

	// retrieve handles to all of the available physical devices
	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, physicalDevices.data());

	// pick a suitable physical device
	for (const auto& physicalDevice : physicalDevices)
	{
		if(isPhysicalDeviceSuitable(physicalDevice))
		{
			mPhysicalDevice = physicalDevice;
		}
	}
	
	if (mPhysicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find a suitable physical device");
	}
}

bool App::isPhysicalDeviceSuitable(VkPhysicalDevice tPhysicalDevice)
{
	auto properties = getPhysicalDeviceProperties(tPhysicalDevice);
	auto features = getPhysicalDeviceFeatures(tPhysicalDevice);

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
		bool supportsAllQueueFlagBits{ true };
		for (const auto &requiredQueueFlagBit : mRequiredQueueFlagBits)
		{
			if (!(queueFamilyProperty.queueFlags & requiredQueueFlagBit))
			{
				supportsAllQueueFlagBits = false;
				break;
			}
		}

		if (supportsAllQueueFlagBits &&
			queueFamilyProperty.queueCount > 0)
		{
			std::cout << "found suitable queue family (index " << queueFamilyIndex << ") with " << queueFamilyProperty.queueCount << " possible queues\n";
			foundSuitableQueueFamily = true;
			break;
		}

		++queueFamilyIndex;
	}

	// make sure that the physical device is a discrete GPU and supports both tessellation and 
	// geometry shaders
	if (foundSuitableQueueFamily &&
		properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		features.tessellationShader &&
		features.geometryShader)
	{
		std::cout << "found suitable physical device:\n";
		std::cout << "\tdevice ID: " << properties.deviceID << "\n";
		std::cout << "\tdevice name: " << properties.deviceName << "\n";
		std::cout << "\tvendor ID: " << properties.vendorID << "\n";
		return true;
	}
	
	return false;
}

VkPhysicalDeviceProperties App::getPhysicalDeviceProperties(VkPhysicalDevice tPhysicalDevice)
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(tPhysicalDevice, &physicalDeviceProperties);

	return physicalDeviceProperties;
}

VkPhysicalDeviceFeatures App::getPhysicalDeviceFeatures(VkPhysicalDevice tPhysicalDevice)
{
	VkPhysicalDeviceFeatures physicalDeviceFeatures;
	vkGetPhysicalDeviceFeatures(tPhysicalDevice, &physicalDeviceFeatures);

	return physicalDeviceFeatures;
}

void App::getPhysicalDeviceMemoryProperties(VkPhysicalDevice tPhysicalDevice)
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
}

std::vector<VkQueueFamilyProperties> App::getPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice tPhysicalDevice)
{
	uint32_t queueFamilyPropertyCount{ 0 };
	vkGetPhysicalDeviceQueueFamilyProperties(tPhysicalDevice, &queueFamilyPropertyCount, nullptr);

	std::cout << "this physical device supports " << queueFamilyPropertyCount << " queue families\n";

	std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(tPhysicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

	return queueFamilyProperties;
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
	deviceCreateInfo.enabledExtensionCount = 0;
	deviceCreateInfo.enabledLayerCount = mRequiredLayers.size();
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.pEnabledFeatures = &mRequiredFeatures;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.ppEnabledExtensionNames = nullptr;
	deviceCreateInfo.ppEnabledLayerNames = mRequiredLayers.data();
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	if (vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mLogicalDevice) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create a logical device");
	}

	std::cout << "sucessfully created a logical device\n";
}

int main()
{
	VkPhysicalDeviceFeatures requiredFeatures = {};

	App myApp{ "test application", requiredFeatures };

	return 0;
}