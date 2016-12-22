#pragma once

#define NOMINMAX
#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <limits>
#include <fstream>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include "glfw3.h"

enum class WindowMode
{
	TOOLKIT_NO_WINDOW,
	TOOLKIT_GLFW_WINDOW
}; 

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR mCapabilities;			// min/max number of images in the swapchain, min/max width and height of the images, etc.
	std::vector<VkSurfaceFormatKHR> mFormats;		// pixel format, color space, etc.
	std::vector<VkPresentModeKHR> mPresentModes;	// presentation modes
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

	App(const std::string &tApplicationName, 
		VkPhysicalDeviceFeatures tRequiredFeatures, 
		WindowMode tWindowMode = WindowMode::TOOLKIT_GLFW_WINDOW) :
		mApplicationName{ tApplicationName },
		mWindowMode{ tWindowMode }
	{
		mRequiredFeatures = tRequiredFeatures; // error if placed in initializer list?
		setup();
	};

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
	std::vector<VkExtensionProperties> getExtensionProperties() const;

	// 2.
	void setupDebugCallback();

	// 3.
	void createSurface();

	// 4.
	void createPhysicalDevice();
	bool isPhysicalDeviceSuitable(VkPhysicalDevice tPhysicalDevice);
	bool checkDeviceExtensionSupport(VkPhysicalDevice tPhysicalDevice);

	SwapchainSupportDetails getSwapchainSupportDetails(VkPhysicalDevice tPhysicalDevice) const;
	VkPhysicalDeviceProperties getPhysicalDeviceProperties(VkPhysicalDevice tPhysicalDevice) const;
	VkPhysicalDeviceFeatures getPhysicalDeviceFeatures(VkPhysicalDevice tPhysicalDevice) const;
	std::vector<VkQueueFamilyProperties> getPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice tPhysicalDevice) const;
	std::vector<VkExtensionProperties> getPhysicalDeviceExtensionProperties(VkPhysicalDevice tPhysicalDevice) const;
	VkPhysicalDeviceMemoryProperties getPhysicalDeviceMemoryProperties(VkPhysicalDevice tPhysicalDevice) const;

	// 5.
	void createLogicalDevice();

	// 6.
	void createSwapchain();
	VkSurfaceFormatKHR selectSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &tSurfaceFormats) const;
	VkPresentModeKHR selectSwapchainPresentMode(const std::vector<VkPresentModeKHR> &tPresentModes) const;
	VkExtent2D selectSwapchainExtent(const VkSurfaceCapabilitiesKHR &tSurfaceCapabilities) const;

	// 7.
	void createImageViews();

	// 8.
	void createRenderPass();

	// 9. 
	void createGraphicsPipeline();
	void createShaderModule(const std::vector<char> &tSrc, VkShaderModule *tShaderModule);

	// 10.
	void createFramebuffers();

	// 11. 
	void createCommandPool();

	// 12. 
	void createCommandBuffers();

	// 13.
	void createSemaphores();

	// app requirements
	std::vector<const char*>		mRequiredLayers = { "VK_LAYER_LUNARG_standard_validation" };				// what Vulkan validation layers does this app need to support?
	std::vector<const char*>		mRequiredExtensions = { VK_EXT_DEBUG_REPORT_EXTENSION_NAME };				// what instance level Vulkan extensions does this app need to support?
	std::vector<const char*>		mRequiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };			// what device level Vulkan extensions does this app need to support?
	VkPhysicalDeviceFeatures		mRequiredFeatures = {};														// what Vulkan features does this app need to support?
	std::vector<VkQueueFlagBits>	mRequiredQueueFlagBits = { VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT };	// what types of operations does this app need to support?
	
	// general app properties
	std::string mApplicationName{ "test application" };
	uint32_t	mWindowWidth{ 640 };
	uint32_t	mWindowHeight{ 480 };
	WindowMode	mWindowMode{ WindowMode::TOOLKIT_GLFW_WINDOW };
	GLFWwindow *mWindowHandle{ nullptr };

	// handles to Vulkan objects
	VkDebugReportCallbackEXT		mDebugCallback{ VK_NULL_HANDLE };
	VkInstance						mInstance{ VK_NULL_HANDLE };
	VkPhysicalDevice				mPhysicalDevice{ VK_NULL_HANDLE };
	VkDevice						mLogicalDevice{ VK_NULL_HANDLE };
	VkQueue							mQueue{ VK_NULL_HANDLE };
	VkSurfaceKHR					mSurface{ VK_NULL_HANDLE };
	VkSwapchainKHR					mSwapchain{ VK_NULL_HANDLE };
	std::vector<VkImage>			mSwapchainImages;
	VkFormat						mSwapchainImageFormat;
	VkExtent2D						mSwapchainImageExtent;
	std::vector<VkImageView>		mSwapchainImageViews;
	VkRenderPass					mRenderPass{ VK_NULL_HANDLE };
	VkPipelineLayout				mPipelineLayout{ VK_NULL_HANDLE };
	VkPipeline						mPipeline{ VK_NULL_HANDLE };
	std::vector<VkFramebuffer>		mSwapchainFramebuffers;
	VkCommandPool					mCommandPool{ VK_NULL_HANDLE };
	std::vector<VkCommandBuffer>	mCommandBuffers;
	VkSemaphore						mImageAvailableSemaphore;
	VkSemaphore						mRenderFinishedSemaphore;

	// other Vulkan related items
	size_t mQueueFamilyIndex;

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags,
														VkDebugReportObjectTypeEXT objType,
														uint64_t obj,
														size_t location,
														int32_t code,
														const char* layerPrefix,
														const char* msg,
														void* userData)
	{
		std::cerr << "validation layer: " << msg << std::endl;
		return VK_FALSE;
	}

};
