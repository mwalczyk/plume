#pragma once

#include <memory>
#include <vector>
#include <iostream>
#include <string>

#include "Platform.h"
#include "Instance.h"
#include "Surface.h"
#include "glfw3.h"

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

		inline GLFWwindow* getWindowHandle() const { return mWindowHandle; }
		inline SurfaceRef getSurface() const {	return mSurface; }
		inline uint32_t getWidth() const { return mWidth; }
		inline uint32_t getHeight() const { return mHeight; }
		inline int shouldWindowClose() const { return glfwWindowShouldClose(mWindowHandle); }
		inline void pollEvents() const { glfwPollEvents(); }

		//! Returns the instance extensions required by the windowing system
		std::vector<const char*> getRequiredInstanceExtensions() const;

	private:

		GLFWwindow *mWindowHandle;

		InstanceRef mInstance;
		SurfaceRef mSurface;

		uint32_t mWidth;
		uint32_t mHeight;
		std::string mTitle;
		Mode mMode;

	};

} // namespace vk