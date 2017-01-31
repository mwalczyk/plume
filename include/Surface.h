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

#include "Platform.h"
#include "Noncopyable.h"
#include "Instance.h"

namespace graphics
{

	class Surface;
	using SurfaceRef = std::shared_ptr<Surface>;

	class Surface : public Noncopyable
	{
	public:

		//! Factory method for returning a new SurfaceRef. Called by the Window class to create a SurfaceRef.
		static SurfaceRef create(const InstanceRef &tInstance)
		{
			return std::make_shared<Surface>(tInstance);
		}

		Surface(const InstanceRef &tInstance);
		~Surface();

		inline VkSurfaceKHR getHandle() const { return mSurfaceHandle; }

	private:

		VkSurfaceKHR mSurfaceHandle;

		InstanceRef mInstance;
		
		friend class Window;
	};

} // namespace graphics