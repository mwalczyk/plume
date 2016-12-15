#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <string>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include "glfw3.h"

enum class WindowMode
{
	TOOLKIT_NO_WINDOW,
	TOOLKIT_GLFW_WINDOW
};

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

	App(const std::string &tApplicationName, VkPhysicalDeviceFeatures tRequiredFeatures, WindowMode tWindowMode = WindowMode::TOOLKIT_GLFW_WINDOW) :
		mApplicationName{ tApplicationName },
		mWindowMode{ tWindowMode }
	{
		mRequiredFeatures = tRequiredFeatures; // error if placed in initializer list?
		setup();
	};

	// public interface
	virtual void setup();
	virtual void update();
	virtual void draw();
	virtual void run() final;

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

	// 5.
	void createSurface();

	// app requirements
	std::vector<const char*> mRequiredLayers = { "VK_LAYER_LUNARG_standard_validation" };					// what Vulkan validation layers does this app need to support?
	std::vector<const char*> mRequiredExtensions = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME };					// what Vulkan extensions does this app need to support?
	VkPhysicalDeviceFeatures mRequiredFeatures = {};														// what Vulkan features does this app need to support?
	std::vector<VkQueueFlagBits> mRequiredQueueFlagBits = { VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT };	// what types of operations does this app need to support?

	// general app properties
	std::string mApplicationName = { "test application" };
	uint32_t mWindowWidth = { 640 };
	uint32_t mWindowHeight = { 480 };
	WindowMode mWindowMode;
	GLFWwindow *mWindowHandle = { nullptr };

	// handles to Vulkan objects
	VkDebugReportCallbackEXT mDebugCallback{ VK_NULL_HANDLE };
	VkInstance mInstance{ VK_NULL_HANDLE };
	VkPhysicalDevice mPhysicalDevice{ VK_NULL_HANDLE };
	VkDevice mLogicalDevice{ VK_NULL_HANDLE };
	VkQueue mQueue{ VK_NULL_HANDLE };
	VkSurfaceKHR mSurface{ VK_NULL_HANDLE };

	// other Vulkan related items
	size_t mQueueFamilyIndex;

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
