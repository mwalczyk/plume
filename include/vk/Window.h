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

#include <memory>
#include <vector>
#include <iostream>
#include <string>
#include <map>

#include "Platform.h"
#include "Noncopyable.h"
#include "Instance.h"
#include "Surface.h"

#include "glfw3.h"
#include "glm.hpp"
#include "gtc/type_ptr.hpp"

namespace graphics
{

	class Window;
	using WindowRef = std::shared_ptr<Window>;

	class Window : public Noncopyable
	{
	public:

		using MouseMovedFuncType = std::function<void(double, double)>;
		using MousePressedFuncType = std::function<void(int, bool, int)>;
		using KeyPressedFuncType = std::function<void(int, int, bool, int)>;
		using ScrollFuncType = std::function<void(double, double)>;

		enum class WindowMode
		{
			WINDOW_MODE_HEADLESS,
			WINDOW_MODE_BORDERS,
			WINDOW_MODE_BORDERLESS,
			WINDOW_MODE_FULLSCREEN
		};

		class Options
		{
		public:

			Options();

			Options& title(const std::string &tTitle) { m_title = tTitle; return *this; }
			Options& resizeable(bool resizeable) { m_resizeable = resizeable; return *this; }
			Options& mode(WindowMode mode) { m_mode = mode; return *this; }

		private:

			std::string m_title;
			bool m_resizeable;
			WindowMode m_mode;

			friend class Window;
		};

		//! Factory method for returning a new WindowRef
		static WindowRef create(const InstanceRef& instance, uint32_t width, uint32_t height, const Options& options = Options())
		{
			return std::make_shared<Window>(instance, width, height, options);
		}

		Window(const InstanceRef& instance, uint32_t width, uint32_t height, const Options& options = Options());
		~Window();

		SurfaceRef create_surface();
		inline GLFWwindow* get_window_handle() const { return m_window_handle; }
		inline uint32_t get_width() const { return m_width; }
		inline uint32_t get_height() const { return m_height; }
		inline const std::string& get_title() const { return m_title; }
		inline void set_title(const std::string& title) { glfwSetWindowTitle(m_window_handle, title.c_str()); }

		//! Returns the instance extensions required by the windowing system
		std::vector<const char*> get_required_instance_extensions() const;

		//! Returns a viewport that corresponds to the full extents of this window.
		vk::Viewport get_fullscreen_viewport() const;

		//! Returns a rect (scissor region) that corresponds to the full extents of this window.
		vk::Rect2D get_fullscreen_scissor_rect2d() const;

		inline int should_close() const { return glfwWindowShouldClose(m_window_handle); }
		inline void poll_events() const { glfwPollEvents(); }
		inline glm::vec2 get_mouse_position() const { double x, y; glfwGetCursorPos(m_window_handle, &x, &y); return { x, y }; }

		//! Add a callback function to this window's mouse moved event.
		inline void connect_to_mouse_moved(const MouseMovedFuncType& connection) { m_mouse_moved_connections.push_back(connection); }
		
		//! Add a callback function to this window's mouse pressed event.
		inline void connect_to_mouse_pressed(const MousePressedFuncType& connection) { m_mouse_pressed_connections.push_back(connection); }
		
		//! Add a callback function to this window's key pressed event.
		inline void connect_to_key_pressed(const KeyPressedFuncType& connection) { m_key_pressed_connections.push_back(connection); }

		//! Add a callback function to this window's scroll event.
		inline void connect_to_scroll(const ScrollFuncType& connection) { m_scroll_connections.push_back(connection); }

	private:

		void initialize_callbacks();
		void on_mouse_moved(double x, double y);
		void on_mouse_pressed(int button, int action, int mods);
		void on_key_pressed(int key, int scancode, int action, int mods);
		void on_scroll(double x_offset, double y_offset);

		InstanceRef m_instance;
		GLFWwindow* m_window_handle;
		uint32_t m_width;
		uint32_t m_height;
		std::string m_title;
		WindowMode m_window_mode;
		std::vector<MouseMovedFuncType> m_mouse_moved_connections;
		std::vector<MousePressedFuncType> m_mouse_pressed_connections;
		std::vector<KeyPressedFuncType> m_key_pressed_connections;
		std::vector<ScrollFuncType> m_scroll_connections;
	};

} // namespace graphics