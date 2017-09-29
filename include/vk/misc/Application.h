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

#include "Vk.h"

namespace plume
{

	namespace app
	{

		class Application
		{
		public:

			Application() = default;
			virtual ~Application() = default;

			virtual void run()
			{
				setup();

				while (!m_window.should_close())
				{
					m_window.poll_events();

					draw();
				}

				exit();
			}

			virtual void setup() { PL_LOG_DEBUG("Setting up application..."); }
			virtual void draw();
			virtual void exit() { PL_LOG_DEBUG("Exiting application..."); }

			virtual const graphics::Instance& get_instance() const final { return m_instance; }
			virtual const graphics::Window& get_window() const final { return m_window; }
			virtual const graphics::Device& get_device() const final { return m_device; }
			virtual const graphics::Swapchain& get_swapchain() const final { return m_swapchain; }
			virtual const inline uint32_t get_width() const final { return m_width; }
			virtual const inline uint32_t get_height() const final { return m_height; }

		private:

			graphics::Instance m_instance;
			graphics::Window m_window;
			graphics::Device m_device;
			graphics::Swapchain m_swapchain;
			uint32_t m_width;
			uint32_t m_height;
		};

		template<class T>
		void run_app()
		{
			Application *app = static_cast<Application*>(new T);
			app->run();
			delete app;
		}

		#define DECLARE_MAIN(ApplicationDerived, ...)	\
		int main()										\
		{												\
			run_app<ApplicationDerived>();				\
		}												

	} // namespace app

} // namespace plume

using namespace pl;
using namespace pl::app;

class ExampleApplication : public Application
{
	// TODO: fill out an example.
};

DECLARE_MAIN(ExampleApplication)