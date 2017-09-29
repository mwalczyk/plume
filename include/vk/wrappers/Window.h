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

#pragma once

#include "Instance.h"

#include "glfw3.h"
#include "glm.hpp"
#include "gtc/type_ptr.hpp"

namespace plume
{

	namespace graphics
	{

		class Window
		{
		public:

			using MouseEnteredFuncType = std::function<void(bool)>;
			using MouseMovedFuncType = std::function<void(double, double)>;
			using MousePressedFuncType = std::function<void(int, bool, int)>;
			using KeyPressedFuncType = std::function<void(int, int, bool, int)>;
			using ScrollFuncType = std::function<void(double, double)>;

			enum class WindowMode
			{
				WINDOW_MODE_HEADLESS,
				WINDOW_MODE_BORDERS,
				WINDOW_MODE_BORDERLESS,
				WINDOW_MODE_FULLSCREEN_BORDERS,
				WINDOW_MODE_FULLSCREEN_BORDERLESS
			};

			Window(const Instance& instance, 
				   uint32_t width, 
				   uint32_t height, 
				   WindowMode mode = WindowMode::WINDOW_MODE_BORDERS, 
				   bool resizeable = false);

			~Window();

			//! Vulkan is a platform agnostic API and therefore does not directly interface with the window system.
			//! The WSI (window system integration) extensions establish the connection between Vulkan and the 
			//! underlying window system to present rendered images to the screen. Note that the surface and window 
			//! creation process is not strictly necessary to build a functional Vulkan system, as Vulkan allows 
			//! headless rendering. 
			//!
			//! Note that the Window class is the sole owner of the surface. It will destroy the surface automatically
			//! upon destruction.
			vk::SurfaceKHR get_surface_handle() const { return m_surface_handle.get(); }

			//! Get a pointer to the underlying GLFW window.
			GLFWwindow* get_window_ptr() const { return m_window_ptr; }

			//! Get the dimensions (width, height) of the underlying GLFW window.
			glm::uvec2 get_dimensions() const { return{ m_width, m_height }; }

			//! Get the width of the underlying GLFW window.
			uint32_t get_width() const { return m_width; }

			//! Get the height of the underlying GLFW window.
			uint32_t get_height() const { return m_height; }

			//! Returns the aspect ratio of the window: width / height.
			float get_aspect_ratio() const { return static_cast<float>(m_width / m_height); }

			//! Sets the position of the underlying GLFW window. Here, `x` and `y` represent the coordinates
			//! of the upper-left hand corner of the window area.
			void set_position(int x, int y) { glfwSetWindowPos(m_window_ptr, x, y); }

			//! Get the title of the underlying GLFW window.
			const std::string& get_title() const { return m_title; }

			//! Set the title of the underlying GLFW window.
			void set_title(const std::string& title) { glfwSetWindowTitle(m_window_ptr, title.c_str()); }

			//! Returns the instance extensions required by the windowing system
			std::vector<const char*> get_required_instance_extensions() const;

			//! Returns a viewport that corresponds to the full extents of this window.
			vk::Viewport get_fullscreen_viewport(float min_depth = 0.0f, float max_depth = 1.0f) const;

			//! Returns a rect (scissor region) that corresponds to the full extents of this window.
			vk::Rect2D get_fullscreen_scissor_rect2d() const;

			//! Returns `true` if the GLFW window has been requested to close.
			int should_close() const { return glfwWindowShouldClose(m_window_ptr); }

			//! Check if any GLFW window events have been triggered.
			void poll_events() const { glfwPollEvents(); }

			//! Returns the xy-coordinates of the mouse. If `clamp_to_window` is `true` (the default
			//! behavior), then the mouse coordinates will be clamped to the range [0..width] and 
			//! [0..height], respectively. If `normalized` is `true`, then the mouse coordinates will
			//! be remapped to the range [0..1] in both the x and y directions.
			glm::vec2 get_mouse_position(bool clamp_to_window = true, bool normalized = false) const
			{
				double x, y;
				glfwGetCursorPos(m_window_ptr, &x, &y);

				if (clamp_to_window)
				{
					x = std::fmin(std::fmax(x, 0.0), static_cast<double>(m_width));
					y = std::fmin(std::fmax(y, 0.0), static_cast<double>(m_height));
				}
				if (normalized)
				{
					x /= static_cast<double>(m_width);
					y /= static_cast<double>(m_height);
				}

				return{ x, y };
			}

			//! Add a callback function to this window's mouse entered event.
			void connect_to_mouse_entered(const MouseEnteredFuncType& connection) { m_mouse_entered_connections.push_back(connection); }

			//! Add a callback function to this window's mouse moved event.
			void connect_to_mouse_moved(const MouseMovedFuncType& connection) { m_mouse_moved_connections.push_back(connection); }

			//! Add a callback function to this window's mouse pressed event.
			void connect_to_mouse_pressed(const MousePressedFuncType& connection) { m_mouse_pressed_connections.push_back(connection); }

			//! Add a callback function to this window's key pressed event.
			void connect_to_key_pressed(const KeyPressedFuncType& connection) { m_key_pressed_connections.push_back(connection); }

			//! Add a callback function to this window's scroll event.
			void connect_to_scroll(const ScrollFuncType& connection) { m_scroll_connections.push_back(connection); }

		private:

			void initialize_callbacks();

			void on_mouse_entered(bool entered);

			void on_mouse_moved(double x, double y);

			void on_mouse_pressed(int button, int action, int mods);

			void on_key_pressed(int key, int scancode, int action, int mods);

			void on_scroll(double x_offset, double y_offset);

			vk::UniqueSurfaceKHR m_surface_handle;

			GLFWwindow* m_window_ptr;
			uint32_t m_width;
			uint32_t m_height;
			WindowMode m_window_mode;
			std::string m_title;

			std::vector<MouseEnteredFuncType> m_mouse_entered_connections;
			std::vector<MouseMovedFuncType> m_mouse_moved_connections;
			std::vector<MousePressedFuncType> m_mouse_pressed_connections;
			std::vector<KeyPressedFuncType> m_key_pressed_connections;
			std::vector<ScrollFuncType> m_scroll_connections;
		};

	} // namespace graphics

} // namespace plume