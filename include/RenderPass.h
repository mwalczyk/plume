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
#include "Image.h"

namespace graphics
{

	class RenderPass;
	using RenderPassRef = std::shared_ptr<RenderPass>;

	//! In Vulkan, a render pass represents a collection of attachments, subpasses, and dependencies between
	//! the subpasses, and describes how the attachments are used over the course of the subpasses. A subpass
	//! represents a phase of rendering that reads and writes a subset of the attachments in a render pass.
	//! Rendering commands are recorded into a particular subpass of a render pass. The specific image views
	//! that will be used for the attachments are specified by framebuffer objects. Framebuffers are created
	//! with respect to a specific render pass that the framebuffer is compatible with.
	class RenderPass : public Noncopyable
	{
	public:

		class Options
		{
		public:

			//! Construct a simple render pass with a subpass that contains a single color attachment and a single
			//! depth attachment.
			Options();

			//! An attachment description describes the properties of an attachment including its format, sample count, and
			//! how its contents ae treated at the beginning and end of each render pass instance.
			Options& attachmentDescriptions(const std::vector<vk::AttachmentDescription> &tAttachmentDescriptions) { mAttachmentDescriptions = tAttachmentDescriptions; return *this; }
			
			//! An attachment reference simply stores an index and a reference to an attachment description. It is used for
			//! constructing a subpass description.
			Options& attachmentReferences(const std::vector<vk::AttachmentReference> &tAttachmentReferences) { mAttachmentReferences = tAttachmentReferences; return *this; }
			
			//! A subpass description describes the subset of attachments that is involved in the execution of each subpass.
			//! Each subpass can read from some attachments as input attachments, write to some as color attachments or 
			//! depth / stencil attachments, and perform multisample resolve operations to resolve attachments.
			Options& subpassDescriptions(const std::vector<vk::SubpassDescription> &tSubpassDescriptions) { mSubpassDescriptions = tSubpassDescriptions; return *this; }

			//! A subpass dependency describes the execution and memory dependencies between subpasses.
			Options& subpassDependencies(const std::vector<vk::SubpassDependency> &tSubpassDependencies) { mSubpassDependencies = tSubpassDependencies; return *this; }

		private:

			std::vector<vk::AttachmentDescription> mAttachmentDescriptions;
			std::vector<vk::AttachmentReference> mAttachmentReferences;
			std::vector<vk::SubpassDescription> mSubpassDescriptions;
			std::vector<vk::SubpassDependency> mSubpassDependencies;

			friend class RenderPass;
		};

		//! Factory method for returning a new RenderPassRef.
		static RenderPassRef create(const DeviceRef &tDevice, const Options &tOptions = Options())
		{
			return std::make_shared<RenderPass>(tDevice, tOptions);
		}

		//! Constructs a generic attachment description and attachment reference for a color attachment with the specified image 
		//! format and attachment index. The numeric index that is used to build each attachment reference corresponds to the 
		//! index of a attachment description in the array that is used to construct the render pass. 
		static std::pair<vk::AttachmentDescription, vk::AttachmentReference> createColorAttachment(const vk::Format &tFormat, uint32_t tAttachment);
		
		//! Constructs a generic attachment description and attachment reference for a depth stencil attachment with the specified 
		//! image format and attachment index. The numeric index that is used to build each attachment reference corresponds to the 
		//! index of a attachment description in the array that is used to construct the render pass. 
		static std::pair<vk::AttachmentDescription, vk::AttachmentReference> createDepthStencilAttachment(const vk::Format &tFormat, uint32_t tAttachment);

		static vk::SubpassDescription createSubpassDescription(const std::vector<vk::AttachmentReference> &tColorAttachmentReferences, const vk::AttachmentReference &tDepthStencilAttachmentReference);
		
		//! This should only be used if the render pass has a single subpass. Internally, this constructs a subpass dependency
		//! whose source subpass is the special keyword VK_SUBPASS_EXTERNAL.
		static vk::SubpassDependency createDefaultSubpassDependency();

		RenderPass(const DeviceRef &tDevice, const Options &tOptions = Options());
		~RenderPass();

		inline vk::RenderPass getHandle() const { return mRenderPassHandle; }

	private:

		DeviceRef mDevice;
		vk::RenderPass mRenderPassHandle;
		std::vector<vk::AttachmentDescription> mAttachmentDescriptions;
		std::vector<vk::AttachmentReference> mAttachmentReferences;
		std::vector<vk::SubpassDescription> mSubpassDescriptions;
		std::vector<vk::SubpassDependency> mSubpassDependencies;
	};

} // namespace graphics