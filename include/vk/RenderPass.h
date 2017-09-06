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

	class RenderPassBuilder
	{
	public:

		struct SubpassRecord
		{
			vk::SubpassDescription build_subpass_description() const
			{
				vk::SubpassDescription subpass_description = {};
				subpass_description.colorAttachmentCount = static_cast<uint32_t>(m_color_attachment_refs.size());
				subpass_description.pColorAttachments = m_color_attachment_refs.data();
				subpass_description.pDepthStencilAttachment = &m_depth_stencil_attachment_ref;
				subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
				subpass_description.pResolveAttachments = m_resolve_attachment_refs.data();

				// TODO: what do these do?
				subpass_description.pInputAttachments = nullptr;
				subpass_description.inputAttachmentCount = 0;
				subpass_description.pPreserveAttachments = nullptr;
				subpass_description.preserveAttachmentCount = 0;

				return subpass_description;
			}

			const vk::SubpassDependency& get_subpass_dependency() const { return m_dependency; }

			std::vector<vk::AttachmentDescription>	m_color_attachment_descs;
			std::vector<vk::AttachmentReference>	m_color_attachment_refs;

			std::vector<vk::AttachmentDescription>	m_resolve_attachment_descs;
			std::vector<vk::AttachmentReference>	m_resolve_attachment_refs;

			vk::AttachmentDescription m_depth_stencil_attachment_desc;
			vk::AttachmentReference m_depth_stencil_attachment_ref;

			vk::SubpassDependency m_dependency;
		};

		RenderPassBuilder() :
			m_is_recording(false)
		{}

		bool is_recording() const { return m_is_recording; }

		void begin_subpass()
		{
			m_is_recording = true;
			m_recorded_subpasses.push_back({});
		}

		void add_color_present_attachment(vk::Format format, uint32_t attachment, uint32_t sample_count = 1)
		{
			// Set up the attachment description.
			vk::AttachmentDescription attachment_description;
			attachment_description.finalLayout = vk::ImageLayout::ePresentSrcKHR;
			attachment_description.format = format;
			attachment_description.initialLayout = vk::ImageLayout::eUndefined;
			attachment_description.loadOp = vk::AttachmentLoadOp::eClear;
			attachment_description.samples = utils::sample_count_to_flags(sample_count);
			attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
			attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
			attachment_description.storeOp = vk::AttachmentStoreOp::eStore;

			// Set up the attachment reference.
			vk::AttachmentReference attachment_reference;
			attachment_reference.attachment = attachment;
			attachment_reference.layout = vk::ImageLayout::eColorAttachmentOptimal;

			m_recorded_subpasses.back().m_color_attachment_descs.push_back(attachment_description);
			m_recorded_subpasses.back().m_color_attachment_refs.push_back(attachment_reference);
		}

		void add_color_transient_attachment(vk::Format format, uint32_t attachment, uint32_t sample_count = 1)
		{
			// Set up the attachment description.

			// For the store op, vk::AttachmentStoreOp::eDontCare is critical, since it allows tile based renderers 
			// to completely avoid writing out the multisampled framebuffer to memory. This is a huge performance and 
			// bandwidth improvement.
			vk::AttachmentDescription attachment_description;
			attachment_description.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
			attachment_description.format = format;
			attachment_description.initialLayout = vk::ImageLayout::eUndefined;
			attachment_description.loadOp = vk::AttachmentLoadOp::eClear;
			attachment_description.samples = utils::sample_count_to_flags(sample_count);
			attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
			attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
			attachment_description.storeOp = vk::AttachmentStoreOp::eDontCare;

			// Set up the attachment reference.
			vk::AttachmentReference attachment_reference;
			attachment_reference.attachment = attachment;
			attachment_reference.layout = vk::ImageLayout::eColorAttachmentOptimal;

			m_recorded_subpasses.back().m_resolve_attachment_descs.push_back(attachment_description);
			m_recorded_subpasses.back().m_resolve_attachment_refs.push_back(attachment_reference);
		}

		void add_depth_stencil_attachment(vk::Format format, uint32_t attachment, uint32_t sample_count = 1)
		{
			if (!utils::is_depth_format(format))
			{
				throw std::runtime_error("Attempting to create a depth stencil attachment with an invalid image format");
			}

			// Set up the attachment description.
			vk::AttachmentDescription attachment_description;
			attachment_description.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			attachment_description.format = format;
			attachment_description.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			attachment_description.loadOp = vk::AttachmentLoadOp::eClear;
			attachment_description.samples = utils::sample_count_to_flags(sample_count);
			attachment_description.stencilLoadOp = utils::is_stencil_format(format) ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare;
			attachment_description.stencilStoreOp = utils::is_stencil_format(format) ? vk::AttachmentStoreOp::eStore : vk::AttachmentStoreOp::eDontCare;
			attachment_description.storeOp = vk::AttachmentStoreOp::eDontCare;

			// Set up the attachment reference.
			vk::AttachmentReference attachment_reference;
			attachment_reference.attachment = attachment;
			attachment_reference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

			m_recorded_subpasses.back().m_depth_stencil_attachment_desc = attachment_description;
			m_recorded_subpasses.back().m_depth_stencil_attachment_ref = attachment_reference;
		}

		static vk::SubpassDependency create_default_subpass_dependency()
		{
			vk::SubpassDependency subpass_dependency;
			subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
			subpass_dependency.dstSubpass = 0;
			subpass_dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			subpass_dependency.srcAccessMask = {};
			subpass_dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
			subpass_dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

			return subpass_dependency;
		}

		void end_subpass(const vk::SubpassDependency dependency = create_default_subpass_dependency())
		{
			m_is_recording = false;

			m_recorded_subpasses.back().m_dependency = dependency;
		}

		const std::vector<SubpassRecord>& get_subpass_records() const { return m_recorded_subpasses; }

	private:

		void bake()
		{

		}

		bool m_is_recording;
		std::vector<SubpassRecord> m_recorded_subpasses;
		std::vector<vk::SubpassDependency> m_recorded_subpass_dependencies;
	};

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
		static RenderPassRef create(DeviceWeakRef device, const RenderPassBuilder& builder)
		{
			return std::make_shared<RenderPass>(device, builder);
		}

		//! Constructs a generic attachment description and attachment reference for a color attachment with the specified image 
		//! format and attachment index. The numeric index that is used to build each attachment reference corresponds to the 
		//! index of a attachment description in the array that is used to construct the render pass. Note that this type of render
		//! pass attachment does not have multisampling enabled. 
		//static std::pair<vk::AttachmentDescription, vk::AttachmentReference> create_color_attachment(vk::Format format, uint32_t attachment);
		
		//! Constructs a generic attachment description and attachment reference for a depth stencil attachment with the specified 
		//! image format and attachment index. The numeric index that is used to build each attachment reference corresponds to the 
		//! index of a attachment description in the array that is used to construct the render pass. 
		//static std::pair<vk::AttachmentDescription, vk::AttachmentReference> create_depth_stencil_attachment(vk::Format format, uint32_t attachment, uint32_t sample_count = 1);

		//! Constructs a generic attachment description and attachment reference for a multisample color attachment with the specified 
		//! image format and attachment index. The numeric index that is used to build each attachment reference corresponds to the 
		//! index of a attachment description in the array that is used to construct the render pass. Note that this type of render pass
		//! attachment is meant to be used with MSAA, as its contents will not be stored between subsequent subpasses for maximum
		//! efficiency.
		//static std::pair<vk::AttachmentDescription, vk::AttachmentReference> create_multisample_attachment(vk::Format format, uint32_t attachment, uint32_t sample_count = 4);

		//static vk::SubpassDescription create_subpass_description(const std::vector<vk::AttachmentReference>& color_attachment_references, const vk::AttachmentReference& depth_stencil_attachment_reference);
		
		//! This should only be used if the render pass has a single subpass. Internally, this constructs a subpass dependency
		//! whose source subpass is the special keyword VK_SUBPASS_EXTERNAL.
		//static vk::SubpassDependency create_default_subpass_dependency();

		RenderPass(DeviceWeakRef, const RenderPassBuilder& builder);

		~RenderPass();

		vk::RenderPass get_handle() const { return m_render_pass_handle; }

	private:

		DeviceWeakRef m_device;
		vk::RenderPass m_render_pass_handle;
		std::vector<vk::AttachmentDescription> m_attachment_descriptions;
		std::vector<vk::AttachmentReference> m_attachment_references;
		std::vector<vk::SubpassDescription> m_subpass_descriptions;
		std::vector<vk::SubpassDependency> m_subpass_dependencies;
	};

} // namespace graphics