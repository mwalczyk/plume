#pragma once

#include <memory>
#include <vector>
#include <iostream>
#include <string>

#include "Platform.h"
#include "Instance.h"
#include "Surface.h"
#include "glfw3.h"
#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/type_ptr.hpp"

namespace vk
{
	

	class Window;
	using WindowRef = std::shared_ptr<Window>;

	class Window
	{
		
	public:

		enum class Mode
		{
			WINDOW_MODE_HEADLESS,
			WINDOW_MODE_BORDERS,
			WINDOW_MODE_FULLSCREEN
		};

		struct Options
		{
			Options();

			Options& width(uint32_t tWidth) { mWidth = tWidth; return *this; }
			Options& height(uint32_t tHeight) { mHeight = tHeight; return *this; }
			Options& title(const std::string &tTitle) { mTitle = tTitle; return *this; }
			Options& resizeable(bool tResizeable) { mResizeable = tResizeable; return *this; }
			Options& mode(Mode tMode) { mMode = tMode; return *this; }

			uint32_t mWidth;
			uint32_t mHeight;
			std::string mTitle;
			bool mResizeable;
			Mode mMode;
		};

		//! Factory method for returning a new WindowRef
		static WindowRef create(const InstanceRef &tInstance, const Options &tOptions = Options())
		{
			return std::make_shared<Window>(tInstance, tOptions);
		}

		Window(const InstanceRef &tInstance, const Options &tOptions = Options());
		~Window();

		SurfaceRef createSurface();
		inline GLFWwindow* getWindowHandle() const { return mWindowHandle; }
		inline uint32_t getWidth() const { return mWidth; }
		inline uint32_t getHeight() const { return mHeight; }

		//! Returns the instance extensions required by the windowing system
		std::vector<const char*> getRequiredInstanceExtensions() const;

		//! Returns a VkViewport structure that corresponds to the full extents of this window.
		VkViewport getFullscreenViewport() const;

		//! Returns a VkRect2D structure (scissor region) that corresponds to the full extents of this window.
		VkRect2D getFullscreenScissorRect2D() const;

		inline int shouldWindowClose() const { return glfwWindowShouldClose(mWindowHandle); }
		inline void pollEvents() const { glfwPollEvents(); }
		inline glm::vec2 getMousePosition() const 
		{ 
			double x;
			double y;
			glfwGetCursorPos(mWindowHandle, &x, &y); 
			return { x, y }; 
		}

	private:

		static void onMouseMoved(GLFWwindow *tWindow, double tX, double tY);

		GLFWwindow *mWindowHandle;

		InstanceRef mInstance;

		uint32_t mWidth;
		uint32_t mHeight;
		std::string mTitle;
		Mode mMode;

	};

} // namespace vk