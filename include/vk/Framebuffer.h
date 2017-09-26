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
#include "RenderPass.h"

namespace graphics
{

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
	class Framebuffer
	{
	public:

		Framebuffer(const Device& device, 
					const RenderPass& render_pass, 
					const std::map<std::string, vk::ImageView>& name_to_image_view_map, 
					uint32_t width, 
					uint32_t height, 
					uint32_t layers = 1);

		vk::Framebuffer get_handle() const { return m_framebuffer_handle.get(); }
		
		uint32_t get_width() const { return m_width; }

		uint32_t get_height() const { return m_height; }

		uint32_t get_layers() const { return m_layers; }

		//! Retrieve the list of image views associated with this framebuffer.
		std::vector<vk::ImageView> get_image_views() const;

		//! Retrieve the list of attachment names associated with this framebuffer.
		std::vector<std::string> get_attachment_names() const;

		//! Retrieve the map of attachment names to image views associated with this framebuffer.
		const std::map<std::string, vk::ImageView>& get_name_to_image_view_map() const { return m_name_to_image_view_map; }

		// TODO: this should validate that the render pass and framebuffer are comptabile
		bool is_compatible(const RenderPass& render_pass);

	private:

		// TODO: this should ensure that all of the image views passed into the constructor correspond to 
		// images with the same dimensions as the framebuffer
		bool check_image_view_dimensions();

		const Device* m_device_ptr;
		vk::UniqueFramebuffer m_framebuffer_handle;

		std::map<std::string, vk::ImageView> m_name_to_image_view_map;
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_layers;
	};

} // namespace graphics