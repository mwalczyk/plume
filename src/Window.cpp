#include "Window.h"

namespace vk
{

	Window::Options::Options()
	{
		mWidth = 640;
		mHeight = 480;
		mTitle = "Window";
		mResizeable = false;
		mMode = Mode::WINDOW_MODE_BORDERS;
	}

	Window::Window(const InstanceRef &tInstance, const Options &tOptions) :
		mInstance(tInstance),
		mWidth(tOptions.mWidth),
		mHeight(tOptions.mHeight),
		mTitle(tOptions.mTitle),
		mMode(tOptions.mMode)
	{
		glfwInit();

		// Disable context creation (only needed for OpenGL / ES not Vulkan)
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		if (tOptions.mResizeable)
		{
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		}
		else
		{
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		}

		mWindowHandle = glfwCreateWindow(mWidth, mHeight, mTitle.c_str(), nullptr, nullptr);

		// Create the surface
		if (mMode != Mode::WINDOW_MODE_HEADLESS)
		{
			mSurface = Surface::create(mInstance);

			// This class is a friend of the Surface class, so we can directly access the VkSurfaceKHR handle.
			if (glfwCreateWindowSurface(mInstance->getHandle(), mWindowHandle, nullptr, &mSurface->mSurfaceHandle) != VK_SUCCESS)
			{
				throw std::runtime_error("Failed to create surface");
			}

			std::cout << "Successfully created surface\n";
		}
	}

	Window::~Window()
	{
		glfwDestroyWindow(mWindowHandle);
	}

	std::vector<const char*> Window::getRequiredInstanceExtensions() const
	{
		uint32_t glfwRequiredExtensionCount = 0;
		const char** glfwRequiredExtensionNames = glfwGetRequiredInstanceExtensions(&glfwRequiredExtensionCount);
		
		// Convert to a vector
		std::vector<const char*> requiredExtensionNames;
		for (size_t i = 0; i < glfwRequiredExtensionCount; ++i)
		{
			requiredExtensionNames.push_back(glfwRequiredExtensionNames[i]);
		}

		return requiredExtensionNames;
	}

} // namespace vk