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

#include "Platform.h"
#include "Noncopyable.h"
#include "Device.h"
#include "Semaphore.h"

namespace graphics
{

	class Swapchain;
	using SwapchainRef = std::shared_ptr<Swapchain>;

	class Swapchain : public Noncopyable
	{
	public:

		class Options
		{
		public:

			Options();
			
			//! Set the preferred presentation mode (defaults is vk::PresentModeKHR::eMailbox).
			Options& presentMode(vk::PresentModeKHR tPresentMode) { mPresentMode = tPresentMode; return *this; }

		private:

			vk::PresentModeKHR mPresentMode;

			friend class Swapchain;
		};

		//! Factory method for returning a new SwapchainRef.
		static SwapchainRef create(const DeviceRef &tDevice, const SurfaceRef &tSurface, uint32_t tWidth, uint32_t tHeight, const Options &tOptions = Options())
		{
			return std::make_shared<Swapchain>(tDevice, tSurface, tWidth, tHeight, tOptions);
		}

		Swapchain(const DeviceRef &tDevice, const SurfaceRef &tSurface, uint32_t tWidth, uint32_t tHeight, const Options &tOptions = Options());
		~Swapchain();

		inline vk::SwapchainKHR getHandle() const { return mSwapchainHandle; };
		inline const std::vector<vk::Image>& getImageHandles() const { return mImageHandles; }
		inline const std::vector<vk::ImageView>& getImageViewHandles() const { return mImageViewHandles; }
		uint32_t acquireNextSwapchainImage(const SemaphoreRef &tSemaphore, uint32_t tNanosecondsTimeout = std::numeric_limits<uint64_t>::max());

	private:

		vk::SurfaceFormatKHR selectSwapchainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &tSurfaceFormats) const;
		vk::PresentModeKHR selectSwapchainPresentMode(const std::vector<vk::PresentModeKHR> &tPresentModes) const;
		vk::Extent2D selectSwapchainExtent(const vk::SurfaceCapabilitiesKHR &tSurfaceCapabilities) const;
		void createImageViews();

		DeviceRef mDevice;
		SurfaceRef mSurface;
		vk::SwapchainKHR mSwapchainHandle;
		vk::Format mSwapchainImageFormat;
		vk::Extent2D mSwapchainImageExtent;
		std::vector<vk::Image> mImageHandles;
		std::vector<vk::ImageView> mImageViewHandles;
		uint32_t mWidth;
		uint32_t mHeight;
	};

} // namespace graphics