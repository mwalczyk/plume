#include "Window.h"

namespace graphics
{

	Window::Options::Options()
	{
		mTitle = "Spectra Application";
		mResizeable = false;
		mWindowMode = WindowMode::WINDOW_MODE_BORDERS;
	}

	Window::Window(const InstanceRef &tInstance, uint32_t tWidth, uint32_t tHeight, const Options &tOptions) :
		mInstance(tInstance),
		mWidth(tWidth),
		mHeight(tHeight),
		mTitle(tOptions.mTitle),
		mWindowMode(tOptions.mWindowMode)
	{
		glfwInit();

		// Disable context creation (only needed for OpenGL / ES not Vulkan).
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		// Disable borders if requested.
		if (tOptions.mWindowMode == WindowMode::WINDOW_MODE_BORDERLESS)
		{
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		}

		// Enable resizing if requested.
		if (tOptions.mResizeable)
		{
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		}
		else
		{
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		}

		mWindowHandle = glfwCreateWindow(mWidth, mHeight, mTitle.c_str(), nullptr, nullptr);

		// Set the GLFW window user pointer to 'this' so that a member function can be used for mouse callbacks. 
		// See: http://stackoverflow.com/questions/7676971/pointing-to-a-function-that-is-a-class-member-glfw-setkeycallback
		glfwSetWindowUserPointer(mWindowHandle, this);
		
		initializeCallbacks();
	}

	Window::~Window()
	{
		glfwDestroyWindow(mWindowHandle);
	}

	SurfaceRef Window::createSurface()
	{
		auto surface = Surface::create(mInstance);

		// This class is a friend of the Surface class, so we can directly access the VkSurfaceKHR handle.
		auto result = glfwCreateWindowSurface(mInstance->getHandle(), mWindowHandle, nullptr, &surface->mSurfaceHandle);
		assert(result == VK_SUCCESS);

		return surface;
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

	VkViewport Window::getFullscreenViewport() const
	{
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = static_cast<float>(mWidth);
		viewport.height = static_cast<float>(mHeight);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		return viewport;
	}

	VkRect2D Window::getFullscreenScissorRect2D() const
	{
		VkRect2D scissor = {};
		scissor.extent = { mWidth, mHeight };
		scissor.offset = { 0, 0 };

		return scissor;
	}

	void Window::onMouseMoved(double tX, double tY)
	{
		for (const auto &connection : mMouseMovedConnections)
		{
			connection(tX, tY);
		}
	}

	void Window::onMousePressed(int tButton, int tAction, int tMods)
	{
		if (tAction == GLFW_REPEAT)
		{
			return;
		}

		bool pressed = (tAction == GLFW_PRESS) ? true : false;

		for (const auto &connection : mMousePressedConnections)
		{
			connection(tButton, pressed, tMods);
		}
	}

	void Window::onKeyPressed(int tKey, int tScancode, int tAction, int tMods)
	{
		if (tAction == GLFW_REPEAT)
		{
			return;
		}

		bool pressed = (tAction == GLFW_PRESS) ? true : false;

		for (const auto &connection : mKeyPressedConnections)
		{
			connection(tKey, tScancode, pressed, tMods);
		}
	}

	void Window::onScroll(double tXOffset, double tYOffset)
	{
		for (const auto &connection : mScrollConnections)
		{
			connection(tXOffset, tYOffset);
		}
	}

	void Window::initializeCallbacks()
	{
		auto mouseMovedProxy = [](GLFWwindow* tWindowHandle, double tX, double tY)
		{
			static_cast<Window*>(glfwGetWindowUserPointer(tWindowHandle))->onMouseMoved(tX, tY);
		};
		glfwSetCursorPosCallback(mWindowHandle, mouseMovedProxy);

		auto mousePressedProxy = [](GLFWwindow* tWindowHandle, int tButton, int tAction, int tMods)
		{
			static_cast<Window*>(glfwGetWindowUserPointer(tWindowHandle))->onMousePressed(tButton, tAction, tMods);
		};
		glfwSetMouseButtonCallback(mWindowHandle, mousePressedProxy);

		auto keyPressedProxy = [](GLFWwindow* tWindowHandle, int tKey, int tScancode, int tAction, int tMods)
		{
			static_cast<Window*>(glfwGetWindowUserPointer(tWindowHandle))->onKeyPressed(tKey, tScancode, tAction, tMods);
		};
		glfwSetKeyCallback(mWindowHandle, keyPressedProxy);

		auto scrollProxy = [](GLFWwindow* tWindowHandle, double tXOffset, double tYOffset)
		{
			static_cast<Window*>(glfwGetWindowUserPointer(tWindowHandle))->onScroll(tXOffset, tYOffset);
		};
		glfwSetScrollCallback(mWindowHandle, scrollProxy);
	}

} // namespace graphics