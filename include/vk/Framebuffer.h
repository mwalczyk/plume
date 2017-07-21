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
#include "RenderPass.h"

namespace graphics
{

	class Framebuffer;
	using FramebufferRef = std::shared_ptr<Framebuffer>;

	//! While render passes describe the structure of subpasses and attachments (independent of any specific
	//! image views for the attachments), framebuffers are a collection of specific image views that will
	//! be used in conjunction with a particular render pass. In other words, framebuffers are created with 
	//! respect to a render pass that the framebuffer is compatible with.
	//!
	//! Two attachment references are compatible if they have matching formats and sample counts. Two arrays
	//! of attachment references are compatible if all corresponding pairs of attachments are compatible. If
	//! the arrays are different lengths, attachment references not present in the small array are treated as
	//! unused (VK_ATTACHMENT_UNUSED).
	//!
	//! A framebuffer is compatible with a render pass if it was created using the same render pass or a 
	//! compatible render pass.
	class Framebuffer : public Noncopyable
	{
	public:

		//! Factory method for returning a new FramebufferRef. Note that any attachment to this framebuffer must
		//! have dimensions at least as large as the framebuffer itself.
		static FramebufferRef create(const DeviceRef& device, const RenderPassRef& render_pass, const std::vector<vk::ImageView>& image_views, uint32_t width, uint32_t height, uint32_t layers = 1)
		{
			return std::make_shared<Framebuffer>(device, render_pass, image_views, width, height, layers);
		}

		Framebuffer(const DeviceRef& device, const RenderPassRef& render_pass, const std::vector<vk::ImageView>& image_views, uint32_t width, uint32_t height, uint32_t layers = 1);
		~Framebuffer();

		inline vk::Framebuffer get_handle() const { return m_framebuffer_handle; };
		inline uint32_t get_width() const { return m_width; }
		inline uint32_t get_height() const { return m_height; }
		inline uint32_t get_layers() const { return m_layers; }

		//! Retrieve the list of attachments associated with this framebuffer.
		inline const std::vector<vk::ImageView>& get_image_views() const { return m_image_views; }

		// TODO: this should validate that the render pass and framebuffer are comptabile
		bool is_compatible(const RenderPassRef& render_pass);

	private:

		// TODO: this should ensure that all of the image views passed into the constructor correspond to 
		// images with the same dimensions as the framebuffer
		bool check_image_view_dimensions();

		DeviceRef m_device;
		RenderPassRef m_render_pass;
		vk::Framebuffer m_framebuffer_handle;
		std::vector<vk::ImageView> m_image_views;
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_layers;
	};

} // namespace graphics