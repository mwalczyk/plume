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