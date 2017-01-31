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
		static FramebufferRef create(const DeviceRef &tDevice, const RenderPassRef &tRenderPass, const std::vector<vk::ImageView> &tImageViews, uint32_t tWidth, uint32_t tHeight, uint32_t tLayers = 1)
		{
			return std::make_shared<Framebuffer>(tDevice, tRenderPass, tImageViews, tWidth, tHeight, tLayers);
		}

		Framebuffer(const DeviceRef &tDevice, const RenderPassRef &tRenderPass, const std::vector<vk::ImageView> &tImageViews, uint32_t tWidth, uint32_t tHeight, uint32_t tLayers = 1);
		~Framebuffer();

		inline vk::Framebuffer getHandle() const { return mFramebufferHandle; };
		inline uint32_t getWidth() const { return mWidth; }
		inline uint32_t getHeight() const { return mHeight; }
		inline uint32_t getLayers() const { return mLayers; }

		//! Retrieve the list of attachments associated with this framebuffer.
		inline const std::vector<vk::ImageView>& getImageViews() const { return mImageViews; }
		bool isCompatible(const RenderPassRef &tRenderPass);

	private:

		DeviceRef mDevice;
		RenderPassRef mRenderPass;
		vk::Framebuffer mFramebufferHandle;
		std::vector<vk::ImageView> mImageViews;
		uint32_t mWidth;
		uint32_t mHeight;
		uint32_t mLayers;
	};

} // namespace graphics