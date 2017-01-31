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

	Framebuffer::Framebuffer(const DeviceRef& device, const RenderPassRef& render_pass, const std::vector<vk::ImageView>& image_views, uint32_t width, uint32_t height, uint32_t layers) :
		m_device(device),
		m_render_pass(render_pass),
		m_image_views(image_views),
		m_width(width),
		m_height(height),
		m_layers(layers)
	{
		vk::FramebufferCreateInfo framebuffer_create_info;
		framebuffer_create_info.attachmentCount = static_cast<uint32_t>(m_image_views.size());
		framebuffer_create_info.height = m_height;
		framebuffer_create_info.layers = m_layers;
		framebuffer_create_info.pAttachments = m_image_views.data();
		framebuffer_create_info.renderPass = m_render_pass->get_handle();
		framebuffer_create_info.width = m_width;

		m_framebuffer_handle = m_device->getHandle().createFramebuffer(framebuffer_create_info);
	}

	Framebuffer::~Framebuffer()
	{
		m_device->getHandle().destroyFramebuffer(m_framebuffer_handle);
	}

} // namespace graphics