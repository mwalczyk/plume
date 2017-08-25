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
		while (!m_window->should_close())
		{
			m_window->poll_events();
			draw();
		}
		exit();
	}

	virtual void setup();
	virtual void draw();
	virtual void exit();
	void set_instance(const graphics::InstanceRef &tInstance) { m_instance = tInstance; }
	void set_window(const graphics::WindowRef &tWindow) { m_window = tWindow; }
	graphics::InstanceRef get_instance() const { return m_instance; }
	graphics::WindowRef get_window() const { return m_window; }
	inline uint32_t get_width() const { return m_width; }
	inline uint32_t get_height() const { return m_height; }

private:

	graphics::InstanceRef m_instance;
	graphics::WindowRef m_window;
	uint32_t m_width;
	uint32_t m_height;
};

#define DECLARE_MAIN(ApplicationDerived, ...)										\						
int main()																			\
{																					\
	graphics::InstanceRef instance = graphics::Instance::create();					\
	graphics::WindowRef window = graphics::Window::create(instance, 800, 800);		\
	Application *application = static_cast<Application*>(new ApplicationDerived);	\
	application->set_instance(instance);											\
	application->set_window(window);												\
	application->setup();															\
	application->run();																\
	delete application;																\
	return 0;																		\
}																					\