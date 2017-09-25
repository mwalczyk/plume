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

#include "Framebuffer.h"

namespace graphics
{

	Framebuffer::Framebuffer(DeviceWeakRef device, 
							 const RenderPassRef& render_pass, 
							 const std::map<std::string, vk::ImageView>& name_to_image_view_map, 
							 uint32_t width, 
							 uint32_t height,
							 uint32_t layers) :
		
		m_device(device),
		m_render_pass(render_pass),
		m_name_to_image_view_map(name_to_image_view_map),
		m_width(width),
		m_height(height),
		m_layers(layers)
	{
		DeviceRef device_shared = m_device.lock();

		// TODO: the ImageView wrapper doesn't work because the swapchain creates its own images and image views that 
		// can't be properly wrapped into the ImageRef and ImageViewRef classes. Maybe the Framebuffer class 
		// should accept the a Swapchain as an additional argument?

		// Make sure that the name passed to the framebuffer corresponds to an attachment in the render 
		// pass instance's builder object.
		const auto& render_pass_attachment_names = m_render_pass->get_render_pass_builder()->get_attachment_names();
		for (const auto& name : get_attachment_names())
		{
			if (std::find(render_pass_attachment_names.begin(),
						  render_pass_attachment_names.end(),
						  name) == render_pass_attachment_names.end())
			{
				throw std::runtime_error("One or more of the attachment names used to construct this FrameBuffer\
							is invalid because it does not correspond to a render pass attachment name.");
			}
		}

		// Note that because a map preserves the order of its keys, we don't need any extra logic here. We want to
		// make sure that the order of the image views below is the same as the corresponding attachment descriptions
		// in the render pass instance. 
		auto image_views = get_image_views();

		vk::FramebufferCreateInfo framebuffer_create_info;
		framebuffer_create_info.attachmentCount = static_cast<uint32_t>(image_views.size());
		framebuffer_create_info.height = m_height;
		framebuffer_create_info.layers = m_layers;
		framebuffer_create_info.pAttachments = image_views.data();
		framebuffer_create_info.renderPass = m_render_pass->get_handle();
		framebuffer_create_info.width = m_width;

		m_framebuffer_handle = device_shared->get_handle().createFramebuffer(framebuffer_create_info);
	}

	Framebuffer::~Framebuffer()
	{
		DeviceRef device_shared = m_device.lock();

		device_shared->get_handle().destroyFramebuffer(m_framebuffer_handle);
	}

	std::vector<vk::ImageView> Framebuffer::get_image_views() const
	{
		std::vector<vk::ImageView> image_views;
		for (const auto& mapping : m_name_to_image_view_map)
		{
			image_views.push_back(mapping.second);
		}
		return image_views;
	}

	std::vector<std::string> Framebuffer::get_attachment_names() const
	{
		std::vector<std::string> attachment_names;
		for (const auto& mapping : m_name_to_image_view_map)
		{
			attachment_names.push_back(mapping.first);
		}
		return attachment_names;
	}

} // namespace graphics