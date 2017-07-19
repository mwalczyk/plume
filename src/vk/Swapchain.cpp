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

#include "Swapchain.h"

namespace graphics
{
	Swapchain::Options::Options()
	{
		m_present_mode = vk::PresentModeKHR::eMailbox;
		m_format = vk::Format::eB8G8R8A8Unorm;
	}

	Swapchain::Swapchain(const DeviceRef& device, const SurfaceRef& surface, uint32_t width, uint32_t height, const Options& options) :
		m_device(device),
		m_surface(surface),
		m_width(width),
		m_height(height)
	{
		auto support_details = m_device->get_swapchain_support_details(m_surface);

		// From the structure above, determine an optimal surface format, presentation mode, and size for the swapchain.
		auto surface_format = select_swapchain_surface_format(support_details.m_formats);
		auto present_mode = select_swapchain_present_mode(support_details.m_present_modes);
		auto extent = select_swapchain_extent(support_details.m_capabilities);

		// If the maxImageCount field is 0, this indicates that there is no limit (besides memory requirements) to the number of images in the swapchain.
		uint32_t image_count = support_details.m_capabilities.minImageCount + 1;
		if (support_details.m_capabilities.maxImageCount > 0 && image_count > support_details.m_capabilities.maxImageCount)
		{
			image_count = support_details.m_capabilities.maxImageCount;
		}

		// For now, we assume that the graphics and presentation queues are the same - this is indicated by the VK_SHARING_MODE_EXCLUSIVE flag.
		// In the future, we will need to account for the fact that these two operations may be a part of different queue families.
		vk::SwapchainCreateInfoKHR swapchain_create_info;
		swapchain_create_info.clipped = VK_TRUE;											// Make sure to ignore pixels that are obscured by other windows.
		swapchain_create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;	// This window should not blend with any other windows in the windowing system.
		swapchain_create_info.imageArrayLayers = 1;									
		swapchain_create_info.imageColorSpace = surface_format.colorSpace;
		swapchain_create_info.imageExtent = extent;
		swapchain_create_info.imageFormat = surface_format.format;
		swapchain_create_info.imageSharingMode = vk::SharingMode::eExclusive;				// This swapchain is only accessed by one queue family (see notes above).
		swapchain_create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
		swapchain_create_info.minImageCount = image_count;
		swapchain_create_info.oldSwapchain = vk::SwapchainKHR{};
		swapchain_create_info.pQueueFamilyIndices = nullptr;								// If the sharing mode is exlusive, we don't need to specify this.
		swapchain_create_info.presentMode = present_mode;
		swapchain_create_info.preTransform = support_details.m_capabilities.currentTransform;
		swapchain_create_info.queueFamilyIndexCount = 0;									// Again, if the sharing mode is exlusive, we don't need to specify this.
		swapchain_create_info.surface = m_surface->get_handle();

		m_swapchain_handle = m_device->get_handle().createSwapchainKHR(swapchain_create_info);

		// Note that the Vulkan implementation may create more swapchain images than requested above - this is why we query the number of images again.
		m_image_handles = m_device->get_handle().getSwapchainImagesKHR(m_swapchain_handle);
	
		// Store the image format and extent for later use.
		m_swapchain_image_format = surface_format.format;
		m_swapchain_image_extent = extent;

		create_image_views();
	}

	Swapchain::~Swapchain()
	{
		m_device->get_handle().destroySwapchainKHR(m_swapchain_handle);
	}

	uint32_t Swapchain::acquire_next_swapchain_image(const SemaphoreRef& semaphore, uint32_t timeout)
	{
		auto result = m_device->get_handle().acquireNextImageKHR(m_swapchain_handle, timeout, semaphore->get_handle(), {});
		return result.value;
	}

	vk::SurfaceFormatKHR Swapchain::select_swapchain_surface_format(const std::vector<vk::SurfaceFormatKHR>& surface_formats) const
	{
		// If there is only one VkSurfaceFormatKHR entry with format vk::Format::eUndefined, this means that the surface has no preferred format,
		// in which case we default to vk::Format::eB8G8R8A8Unorm and vk::ColorSpaceKHR::eSrgbNonlinear.
		if (surface_formats.size() == 1 &&
			surface_formats[0].format == vk::Format::eUndefined)
		{
			return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
		}

		// Otherwise, there is a preferred format - iterate through and see if the above combination is available.
		for (const auto& surface_format : surface_formats)
		{
			if (surface_format.format == vk::Format::eB8G8R8A8Unorm &&
				surface_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
			{
				return surface_format;
			}
		}

		// At this point, we could start ranking the available formats and determine which one is "best."
		// For now, return the first available format, since our preferred format was not available.
		return surface_formats[0];
	}

	vk::PresentModeKHR Swapchain::select_swapchain_present_mode(const std::vector<vk::PresentModeKHR>& present_modes) const
	{
		// The swapchain can use one of the following modes for presentation:
		// vk::PresentModeKHR::eImmediate
		// vk::PresentModeKHR::eFifo (the only mode guaranteed to be available)
		// vk::PresentModeKHR::eFifoRelaxed
		// vk::PresentModeKHR::eMailbox
		for (const auto& mode : present_modes)
		{
			if (mode == vk::PresentModeKHR::eMailbox)
			{
				return mode;
			}
		}
		
		// This present mode is always available - use it if the preferred mode is not found.
		return vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D Swapchain::select_swapchain_extent(const vk::SurfaceCapabilitiesKHR& surface_capabilities) const
	{
		if (surface_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return surface_capabilities.currentExtent;
		}
		else
		{
			vk::Extent2D actual_extent = { m_width, m_height };
			actual_extent.width = std::max(surface_capabilities.minImageExtent.width, std::min(surface_capabilities.maxImageExtent.width, actual_extent.width));
			actual_extent.height = std::max(surface_capabilities.minImageExtent.height, std::min(surface_capabilities.maxImageExtent.height, actual_extent.height));
			return actual_extent;
		}
	}

	void Swapchain::create_image_views()
	{
		m_image_view_handles.resize(m_image_handles.size());

		for (size_t i = 0; i < m_image_view_handles.size(); ++i)
		{
			vk::ImageViewCreateInfo image_view_create_info;
			image_view_create_info.components.a = vk::ComponentSwizzle::eIdentity;					// For now, do not swizzle any of the color channels.
			image_view_create_info.components.b = vk::ComponentSwizzle::eIdentity;
			image_view_create_info.components.g = vk::ComponentSwizzle::eIdentity;
			image_view_create_info.components.r = vk::ComponentSwizzle::eIdentity;
			image_view_create_info.format = m_swapchain_image_format;
			image_view_create_info.image = m_image_handles[i];
			image_view_create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;	// This describes the image's purpose - we will be using these images as color targets.
			image_view_create_info.subresourceRange.baseArrayLayer = 0;								// This describes which part of the image we will access.
			image_view_create_info.subresourceRange.baseMipLevel = 0;
			image_view_create_info.subresourceRange.layerCount = 1;
			image_view_create_info.subresourceRange.levelCount = 1;
			image_view_create_info.viewType = vk::ImageViewType::e2D;								// Treat the image as a standard 2D texture.

			m_image_view_handles[i] = m_device->get_handle().createImageView(image_view_create_info);
		}
	}

} // namespace graphics