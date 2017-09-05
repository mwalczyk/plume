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
#include "Image.h"

namespace graphics
{

	class Swapchain;
	using SwapchainRef = std::shared_ptr<Swapchain>;

	class Swapchain : public Noncopyable
	{
	public:

		//! Factory method for returning a new SwapchainRef. This constructor will automatically
		//! choose the optimal swapchain image format and presentation mode.
		static SwapchainRef create(DeviceWeakRef device, 
			const SurfaceRef& surface, 
			uint32_t width, 
			uint32_t height)
		{
			return std::make_shared<Swapchain>(device, surface, width, height);
		}

		Swapchain(DeviceWeakRef device, 
			const SurfaceRef& surface,
			uint32_t width,
			uint32_t height);
		
		~Swapchain();

		vk::SwapchainKHR get_handle() const { return m_swapchain_handle; };
		
		const std::vector<vk::Image>& get_image_handles() const { return m_image_handles; }
		
		const std::vector<vk::ImageView>& get_image_view_handles() const { return m_image_view_handles; }
		
		size_t get_image_count() const { return m_image_handles.size(); }
		
		vk::Extent2D get_image_extent() const { return m_swapchain_image_extent; }
		
		vk::Format get_image_format() const { return m_swapchain_image_format; }
		
		uint32_t acquire_next_swapchain_image(const SemaphoreRef& semaphore, uint32_t timeout = std::numeric_limits<uint64_t>::max());

	private:

		vk::SurfaceFormatKHR select_swapchain_surface_format(const std::vector<vk::SurfaceFormatKHR>& surface_formats) const;
		
		vk::PresentModeKHR select_swapchain_present_mode(const std::vector<vk::PresentModeKHR>& present_modes) const;
		
		vk::Extent2D select_swapchain_extent(const vk::SurfaceCapabilitiesKHR& surface_capabilities) const;
		
		void create_image_views();

		DeviceWeakRef m_device;
		SurfaceRef m_surface;
		vk::SwapchainKHR m_swapchain_handle;
		std::vector<vk::Image> m_image_handles;
		std::vector<vk::ImageView> m_image_view_handles;
		vk::Format m_swapchain_image_format;
		vk::Extent2D m_swapchain_image_extent;
		uint32_t m_width;
		uint32_t m_height;
	};

} // namespace graphics