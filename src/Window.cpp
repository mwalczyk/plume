/*
*
* MIT License
*
* Copyright(c) 2017 Michael Walczyk
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files(the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions :
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*/

#include "Window.h"

namespace graphics
{

	Window::Options::Options()
	{
		m_title = "Spectrum Application";
		m_resizeable = false;
		m_mode = WindowMode::WINDOW_MODE_BORDERS;
	}

	Window::Window(const InstanceRef& instance, uint32_t width, uint32_t height, const Options& options) :
		m_instance(instance),
		m_width(width),
		m_height(height),
		m_title(options.m_title),
		m_window_mode(options.m_mode)
	{
		glfwInit();

		// Disable context creation (only needed for OpenGL / ES not Vulkan).
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		// Disable borders if requested.
		if (m_window_mode == WindowMode::WINDOW_MODE_BORDERLESS)
		{
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		}

		// Enable resizing if requested.
		if (options.m_resizeable)
		{
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		}
		else
		{
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		}

		m_window_handle = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);

		// Set the GLFW window user pointer to 'this' so that a member function can be used for mouse callbacks. 
		// See: http://stackoverflow.com/questions/7676971/pointing-to-a-function-that-is-a-class-member-glfw-setkeycallback
		glfwSetWindowUserPointer(m_window_handle, this);
		
		initialize_callbacks();
	}

	Window::~Window()
	{
		glfwDestroyWindow(m_window_handle);
	}

	SurfaceRef Window::create_surface()
	{
		auto surface = Surface::create(m_instance);

		// This class is a friend of the Surface class, so we can directly access the VkSurfaceKHR handle.
		glfwCreateWindowSurface(m_instance->get_handle(), m_window_handle, nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface->m_surface_handle));

		return surface;
	}

	std::vector<const char*> Window::get_required_instance_extensions() const
	{
		uint32_t glfw_required_extension_count = 0;
		const char** glfw_required_extension_names = glfwGetRequiredInstanceExtensions(&glfw_required_extension_count);
		
		// Convert to a vector
		std::vector<const char*> required_extension_names;
		for (size_t i = 0; i < glfw_required_extension_count; ++i)
		{
			required_extension_names.push_back(glfw_required_extension_names[i]);
		}

		return required_extension_names;
	}

	vk::Viewport Window::get_fullscreen_viewport() const
	{
		vk::Viewport viewport;
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = static_cast<float>(m_width);
		viewport.height = static_cast<float>(m_height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		return viewport;
	}

	vk::Rect2D Window::get_fullscreen_scissor_rect2d() const
	{
		vk::Rect2D scissor;
		scissor.extent = { m_width, m_height };
		scissor.offset = { 0, 0 };

		return scissor;
	}

	void Window::on_mouse_moved(double x, double y)
	{
		for (const auto &connection : m_mouse_moved_connections)
		{
			connection(x, y);
		}
	}

	void Window::on_mouse_pressed(int button, int action, int mods)
	{
		if (action == GLFW_REPEAT)
		{
			return;
		}

		bool pressed = (action == GLFW_PRESS) ? true : false;

		for (const auto &connection : m_mouse_pressed_connections)
		{
			connection(button, pressed, mods);
		}
	}

	void Window::on_key_pressed(int key, int scancode, int action, int mods)
	{
		if (action == GLFW_REPEAT)
		{
			return;
		}

		bool pressed = (action == GLFW_PRESS) ? true : false;

		for (const auto &connection : m_key_pressed_connections)
		{
			connection(key, scancode, pressed, mods);
		}
	}

	void Window::on_scroll(double x_offset, double y_offset)
	{
		for (const auto &connection : m_scroll_connections)
		{
			connection(x_offset, y_offset);
		}
	}

	void Window::initialize_callbacks()
	{
		auto mouse_moved_proxy = [](GLFWwindow* handle, double x, double y)
		{
			static_cast<Window*>(glfwGetWindowUserPointer(handle))->on_mouse_moved(x, y);
		};
		glfwSetCursorPosCallback(m_window_handle, mouse_moved_proxy);

		auto mouse_pressed_proxy = [](GLFWwindow* handle, int button, int action, int mods)
		{
			static_cast<Window*>(glfwGetWindowUserPointer(handle))->on_mouse_pressed(button, action, mods);
		};
		glfwSetMouseButtonCallback(m_window_handle, mouse_pressed_proxy);

		auto key_pressed_proxy = [](GLFWwindow* handle, int key, int scancode, int action, int mods)
		{
			static_cast<Window*>(glfwGetWindowUserPointer(handle))->on_key_pressed(key, scancode, action, mods);
		};
		glfwSetKeyCallback(m_window_handle, key_pressed_proxy);

		auto scroll_proxy = [](GLFWwindow* handle, double x_offset, double y_offset)
		{
			static_cast<Window*>(glfwGetWindowUserPointer(handle))->on_scroll(x_offset, y_offset);
		};
		glfwSetScrollCallback(m_window_handle, scroll_proxy);
	}

} // namespace graphics