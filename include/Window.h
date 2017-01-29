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
#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/type_ptr.hpp"

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

			Options& title(const std::string &tTitle) { mTitle = tTitle; return *this; }
			Options& resizeable(bool tResizeable) { mResizeable = tResizeable; return *this; }
			Options& mode(WindowMode tWindowMode) { mWindowMode = tWindowMode; return *this; }

		private:

			std::string mTitle;
			bool mResizeable;
			WindowMode mWindowMode;

			friend class Window;
		};

		//! Factory method for returning a new WindowRef
		static WindowRef create(const InstanceRef &tInstance, uint32_t tWidth, uint32_t tHeight, const Options &tOptions = Options())
		{
			return std::make_shared<Window>(tInstance, tWidth, tHeight, tOptions);
		}

		Window(const InstanceRef &tInstance, uint32_t tWidth, uint32_t tHeight, const Options &tOptions = Options());
		~Window();

		SurfaceRef createSurface();
		inline GLFWwindow* getWindowHandle() const { return mWindowHandle; }
		inline uint32_t getWidth() const { return mWidth; }
		inline uint32_t getHeight() const { return mHeight; }
		inline const std::string& getTitle() const { return mTitle; }
		inline void setTitle(const std::string &tTitle) { glfwSetWindowTitle(mWindowHandle, tTitle.c_str()); }

		//! Returns the instance extensions required by the windowing system
		std::vector<const char*> getRequiredInstanceExtensions() const;

		//! Returns a VkViewport structure that corresponds to the full extents of this window.
		VkViewport getFullscreenViewport() const;

		//! Returns a VkRect2D structure (scissor region) that corresponds to the full extents of this window.
		VkRect2D getFullscreenScissorRect2D() const;

		inline int shouldWindowClose() const { return glfwWindowShouldClose(mWindowHandle); }
		inline void pollEvents() const { glfwPollEvents(); }
		inline glm::vec2 getMousePosition() const { double x, y; glfwGetCursorPos(mWindowHandle, &x, &y); return { x, y }; }

		//! Add a callback function to this window's mouse moved event.
		inline void connectToMouseMoved(const MouseMovedFuncType &tConnection) { mMouseMovedConnections.push_back(tConnection); }
		
		//! Add a callback function to this window's mouse pressed event.
		inline void connectToMousePressed(const MousePressedFuncType &tConnection) { mMousePressedConnections.push_back(tConnection); }
		
		//! Add a callback function to this window's key pressed event.
		inline void connectToKeyPressed(const KeyPressedFuncType &tConnection) { mKeyPressedConnections.push_back(tConnection); }

		//! Add a callback function to this window's scroll event.
		inline void connectToScroll(const ScrollFuncType &tConnection) { mScrollConnections.push_back(tConnection); }

	private:

		void initializeCallbacks();
		void onMouseMoved(double tX, double tY);
		void onMousePressed(int tButton, int tAction, int tMods);
		void onKeyPressed(int tKey, int tScancode, int tAction, int tMods);
		void onScroll(double tXOffset, double tYOffset);

		InstanceRef mInstance;
		GLFWwindow *mWindowHandle;
		uint32_t mWidth;
		uint32_t mHeight;
		std::string mTitle;
		WindowMode mWindowMode;
		std::vector<MouseMovedFuncType> mMouseMovedConnections;
		std::vector<MousePressedFuncType> mMousePressedConnections;
		std::vector<KeyPressedFuncType> mKeyPressedConnections;
		std::vector<ScrollFuncType> mScrollConnections;
	};

} // namespace graphics