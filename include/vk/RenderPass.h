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
	//! represents a phase of rendering that reads and writes to a subset of the attachments in a render pass.
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
			Options& attachment_descriptions(const std::vector<vk::AttachmentDescription>& attachment_descriptions) { m_attachment_descriptions = attachment_descriptions; return *this; }
			
			//! An attachment reference simply stores an index and a reference to an attachment description. It is used for
			//! constructing a subpass description.
			Options& attachment_references(const std::vector<vk::AttachmentReference>& attachment_references) { m_attachment_references = attachment_references; return *this; }
			
			//! A subpass description describes the subset of attachments that is involved in the execution of each subpass.
			//! Each subpass can read from some attachments as input attachments, write to some as color attachments or 
			//! depth / stencil attachments, and perform multisample resolve operations to resolve attachments.
			Options& subpass_descriptions(const std::vector<vk::SubpassDescription>& subpass_descriptions) { m_subpass_descriptions = subpass_descriptions; return *this; }

			//! A subpass dependency describes the execution and memory dependencies between subpasses.
			Options& subpass_dependencies(const std::vector<vk::SubpassDependency>& subpass_dependencies) { m_subpass_dependencies = subpass_dependencies; return *this; }

		private:

			std::vector<vk::AttachmentDescription> m_attachment_descriptions;
			std::vector<vk::AttachmentReference> m_attachment_references;
			std::vector<vk::SubpassDescription> m_subpass_descriptions;
			std::vector<vk::SubpassDependency> m_subpass_dependencies;

			friend class RenderPass;
		};

		//! Factory method for returning a new RenderPassRef.
		static RenderPassRef create(const DeviceRef& device, const Options& options = Options())
		{
			return std::make_shared<RenderPass>(device, options);
		}

		//! Constructs a generic attachment description and attachment reference for a color attachment with the specified image 
		//! format and attachment index. The numeric index that is used to build each attachment reference corresponds to the 
		//! index of a attachment description in the array that is used to construct the render pass. Note that this type of render
		//! pass attachment does not have multisampling enabled. 
		static std::pair<vk::AttachmentDescription, vk::AttachmentReference> create_color_attachment(vk::Format format, uint32_t attachment);
		
		//! Constructs a generic attachment description and attachment reference for a depth stencil attachment with the specified 
		//! image format and attachment index. The numeric index that is used to build each attachment reference corresponds to the 
		//! index of a attachment description in the array that is used to construct the render pass. 
		static std::pair<vk::AttachmentDescription, vk::AttachmentReference> create_depth_stencil_attachment(vk::Format format, uint32_t attachment, uint32_t sample_count = 1);

		//! Constructs a generic attachment description and attachment reference for a multisample color attachment with the specified 
		//! image format and attachment index. The numeric index that is used to build each attachment reference corresponds to the 
		//! index of a attachment description in the array that is used to construct the render pass. Note that this type of render pass
		//! attachment is meant to be used with MSAA, as its contents will not be stored between subsequent subpasses for maximum
		//! efficiency.
		static std::pair<vk::AttachmentDescription, vk::AttachmentReference> create_multisample_attachment(vk::Format format, uint32_t attachment, uint32_t sample_count = 4);

		static vk::SubpassDescription create_subpass_description(const std::vector<vk::AttachmentReference>& color_attachment_references, const vk::AttachmentReference& depth_stencil_attachment_reference);
		
		//! This should only be used if the render pass has a single subpass. Internally, this constructs a subpass dependency
		//! whose source subpass is the special keyword VK_SUBPASS_EXTERNAL.
		static vk::SubpassDependency create_default_subpass_dependency();

		RenderPass(const DeviceRef& device, const Options& options = Options());
		~RenderPass();

		inline vk::RenderPass get_handle() const { return m_render_pass_handle; }

	private:

		DeviceRef m_device;
		vk::RenderPass m_render_pass_handle;
		std::vector<vk::AttachmentDescription> m_attachment_descriptions;
		std::vector<vk::AttachmentReference> m_attachment_references;
		std::vector<vk::SubpassDescription> m_subpass_descriptions;
		std::vector<vk::SubpassDependency> m_subpass_dependencies;
	};

} // namespace graphics