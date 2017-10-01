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

#include "Device.h"
#include "Image.h"
#include "Synchronization.h"

namespace plume
{

	namespace graphics
	{

		class Swapchain
		{
		public:

			Swapchain(const Device& device, vk::SurfaceKHR surface, uint32_t width, uint32_t height);

			~Swapchain();

			vk::SwapchainKHR get_handle() const { return m_swapchain_handle.get(); };

			const std::vector<vk::Image>& get_image_handles() const { return m_image_handles; }

			const std::vector<vk::ImageView>& get_image_view_handles() const { return m_image_view_handles; }

			//! Returns the number of images in the swapchain.
			size_t get_image_count() const { return m_image_handles.size(); }

			//! Returns the extent (width and height) of each image in the swapchain.
			vk::Extent2D get_image_extent() const { return m_swapchain_image_extent; }

			//! Returns the format of each image in the swapchain.
			vk::Format get_image_format() const { return m_swapchain_image_format; }

		private:

			//! Given a vector of available surface formats, choose the optimal one.
			vk::SurfaceFormatKHR select_swapchain_surface_format(const std::vector<vk::SurfaceFormatKHR>& surface_formats) const;

			//! Given a vector of available presentation modes, choose the optimal one.
			vk::PresentModeKHR select_swapchain_present_mode(const std::vector<vk::PresentModeKHR>& present_modes) const;

			//! Given the capabilities of the provided surface, choose the final extent of each image in the swapchain.
			vk::Extent2D select_swapchain_extent(const vk::SurfaceCapabilitiesKHR& surface_capabilities) const;

			//! Build all of the image views for the images in the swapchain.
			void create_image_views();

			const Device* m_device_ptr;
			vk::UniqueSwapchainKHR m_swapchain_handle;

			std::vector<vk::Image> m_image_handles;
			std::vector<vk::ImageView> m_image_view_handles;
			vk::Format m_swapchain_image_format;
			vk::Extent2D m_swapchain_image_extent;
			uint32_t m_width;
			uint32_t m_height;
		};

	} // namespace graphics

} // namespace plume