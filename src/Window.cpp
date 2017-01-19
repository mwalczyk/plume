#include "Window.h"

namespace vk
{

	void Window::onMouseMoved(GLFWwindow* tWindow, double tX, double tY)
	{
	}

	Window::Options::Options()
	{
		mTitle = "Spectra Application";
		mResizeable = false;
		mMode = Mode::WINDOW_MODE_BORDERS;
	}

	Window::Window(const InstanceRef &tInstance, uint32_t tWidth, uint32_t tHeight, const Options &tOptions) :
		mInstance(tInstance),
		mWidth(tWidth),
		mHeight(tHeight),
		mTitle(tOptions.mTitle),
		mMode(tOptions.mMode)
	{
		glfwInit();

		// Disable context creation (only needed for OpenGL / ES not Vulkan).
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		// Disable borders if requested.
		if (tOptions.mMode == Mode::WINDOW_MODE_BORDERLESS)
		{
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		}

		// Enable resizing if requested.
		if (tOptions.mResizeable)
		{
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		}
		else
		{
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		}

		mWindowHandle = glfwCreateWindow(mWidth, mHeight, mTitle.c_str(), nullptr, nullptr);

		glfwSetCursorPosCallback(mWindowHandle, (GLFWcursorposfun) onMouseMoved);
	}

	Window::~Window()
	{
		glfwDestroyWindow(mWindowHandle);
	}

	SurfaceRef Window::createSurface()
	{
		auto surface = Surface::create(mInstance);

		// This class is a friend of the Surface class, so we can directly access the VkSurfaceKHR handle.
		if (glfwCreateWindowSurface(mInstance->getHandle(), mWindowHandle, nullptr, &surface->mSurfaceHandle) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create surface");
		}

		std::cout << "Successfully created surface\n";

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
		viewport.width = mWidth;
		viewport.height = mHeight;
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

} // namespace vk