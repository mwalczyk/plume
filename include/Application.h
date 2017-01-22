#pragma once

#include "Instance.h"
#include "Window.h"

class Application
{
public:

	Application() = default;

	virtual void run()
	{
		while (!mWindow->shouldWindowClose())
		{
			mWindow->pollEvents();
			draw();
		}
		exit();
	}

	virtual void setup();
	virtual void draw();
	virtual void exit();
	void setInstance(const vk::InstanceRef &tInstance) { mInstance = tInstance; }
	void setWindow(const vk::WindowRef &tWindow) { mWindow = tWindow; }
	vk::InstanceRef getInstance() const { return mInstance; }
	vk::WindowRef getWindow() const { return mWindow; }
	inline uint32_t getWidth() const { return mWidth; }
	inline uint32_t getHeight() const { return mHeight; }

private:

	vk::InstanceRef mInstance;
	vk::WindowRef mWindow;
	uint32_t mWidth;
	uint32_t mHeight;
};

#define DECLARE_MAIN(ApplicationDerived, ...)										\						
int main()																			\
{																					\
	vk::InstanceRef instance = vk::Instance::create();								\
	vk::WindowRef window = vk::Window::create(instance, 800, 800);					\
	Application *application = static_cast<Application*>(new ApplicationDerived);	\
	application->setInstance(instance);												\
	application->setWindow(window);													\
	application->setup();															\
	application->run();																\
	delete application;																\
	return 0;																		\
}																					\