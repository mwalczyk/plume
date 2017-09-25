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

namespace graphics
{

	class Surface;
	using SurfaceRef = std::shared_ptr<Surface>;

	//! Vulkan is a platform agnostic API and therefore does not directly interface with the window system.
	//! The WSI (window system integration) extensions establish the connection between Vulkan and the 
	//! underlying window system to present rendered images to the screen. Note that the surface and window 
	//! creation process is not strictly necessary to build a functional Vulkan system, as Vulkan allows 
	//! headless rendering.
	class Surface : public Noncopyable
	{
	public:

		//! Factory method for returning a new SurfaceRef. Called by the Window class to create a SurfaceRef.
		static SurfaceRef create(const InstanceRef& instance)
		{
			return std::make_shared<Surface>(instance);
		}

		Surface(const InstanceRef& instance);
		
		~Surface();

	    vk::SurfaceKHR get_handle() const { return m_surface_handle; }

	private:

		InstanceRef m_instance;
		vk::SurfaceKHR m_surface_handle;

		friend class Window;
	};

} // namespace graphics