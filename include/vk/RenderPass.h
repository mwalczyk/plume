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
#include <map>

#include "Platform.h"
#include "Noncopyable.h"
#include "Device.h"
#include "Image.h"

namespace graphics
{

	enum class AttachmentCategory
	{
		CATEGORY_COLOR,
		CATEGORY_RESOLVE,
		CATEGORY_DEPTH_STENCIL,
		CATEGORY_INPUT,
		CATEGORY_PRESERVE
	};

	class RenderPassBuilder;
	using RenderPassBuilderRef = std::shared_ptr<RenderPassBuilder>;

	//! A helper class for constructing render passes.
	//!
	//! Each vk::AttachmentDescription describes a particular attachment that will be used during the
	//! render pass. It has several important fields:
	//!
	//! - flags: only possible value is vk::AttachmentDescriptionFlagBits::eMayAlias, which says that
	//!   this attachment aliases the same device memory as other attachments
	//! - format: the format of the image that will be used for the attachment
	//! - samples: the number of samples of the image
	//! - loadOp: how the contents of color and depth components are treated at the beginning of the
	//!   subpass where it is first used
	//! - storeOp: how the contents of color and depth components are treated at the end of the 
	//!   subpass where it is last used
	//! - stencilLoadOp: same as `loadOp` but for the stencil component
	//! - stencilStoreOp: same as `storeOp` but for the stencil component
	//! - initialLayout: the layout the attachment image subresource will be in when a render pass
	//!   instance begins
	//! - finalLayout: the layout the attachment image subresource will be transitioned to when a 
	//!   render pass instance ends - note that an attachment can use a different layout in each
	//!   subpass, if desired
	//!
	//! Each vk::AttachmentReference simply stores an index and a reference to an attachment description. 
	//! It is used for constructing a subpass description.
	//!
	//! Each vk::SubpassDescription describes the subset of attachments that is involved in the execution of 
	//! a subpass. Each subpass can read from some attachments as input attachments, write to some as color 
	//! attachments or depth / stencil attachments, and perform multisample resolve operations to resolve 
	//! attachments.
	//!
	//! Each vk::SubpassDependency describes the execution and memory dependencies between subpasses.
	class RenderPassBuilder
	{
	public: 

		static RenderPassBuilderRef create()
		{
			return std::make_shared<RenderPassBuilder>();
		}

		RenderPassBuilder() :
			m_is_recording(false)
		{}

		//! Constructs an attachment description for a generic attachment.
		void add_generic_attachment(const std::string& name, const vk::AttachmentDescription& attachment_description)
		{
			check_attachment_name_unique(name);

			// Add to global map of string names to attachment descriptions.
			m_attachment_mapping.insert({ name, attachment_description });
		}

		//! Constructs an attachment description for a generic attachment.
		void add_generic_attachment(const std::string& name, 
									vk::Format format =							vk::Format::eB8G8R8A8Unorm,
									uint32_t sample_count =						1,
									vk::ImageLayout initial_layout =			vk::ImageLayout::eUndefined,
									vk::ImageLayout final_layout =				vk::ImageLayout::eGeneral,
									vk::AttachmentLoadOp load_op =				vk::AttachmentLoadOp::eDontCare,
									vk::AttachmentStoreOp store_op =			vk::AttachmentStoreOp::eDontCare,
									vk::AttachmentLoadOp stencil_load_op =		vk::AttachmentLoadOp::eDontCare,
									vk::AttachmentStoreOp stencil_store_op =	vk::AttachmentStoreOp::eDontCare)
		{
			check_attachment_name_unique(name);

			vk::AttachmentDescription attachment_description;
			attachment_description.finalLayout = final_layout;
			attachment_description.format = format;
			attachment_description.initialLayout = initial_layout;
			attachment_description.loadOp = load_op;
			attachment_description.samples = utils::sample_count_to_flags(sample_count);
			attachment_description.stencilLoadOp = stencil_load_op;
			attachment_description.stencilStoreOp = stencil_store_op;
			attachment_description.storeOp = store_op;

			// Add to global map of string names to attachment descriptions.
			m_attachment_mapping.insert({ name, attachment_description });
		}

		//! Constructs an attachment description for a color attachment with the specified image 
		//! format and sample count. The final layout of this attachment will be vk::ImageLayout::ePresentSrcKHR,
		//! so this method should only be called for an attachment that will (eventually) be presented back to 
		//! the swapchain.
		void add_color_present_attachment(const std::string& name, vk::Format format, uint32_t sample_count = 1)
		{
			check_attachment_name_unique(name);

			vk::AttachmentDescription attachment_description;
			attachment_description.finalLayout = vk::ImageLayout::ePresentSrcKHR;
			attachment_description.format = format;
			attachment_description.initialLayout = vk::ImageLayout::eUndefined;
			attachment_description.loadOp = vk::AttachmentLoadOp::eClear;
			attachment_description.samples = utils::sample_count_to_flags(sample_count);
			attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
			attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
			attachment_description.storeOp = vk::AttachmentStoreOp::eStore;

			// Add to global map of string names to attachment descriptions.
			m_attachment_mapping.insert({ name, attachment_description });
		}

		//! Constructs an attachment description for a multisample color attachment with the specified image format and sample 
		//! count. Note that this type of render pass attachment is meant to be used with MSAA, as its contents will not be stored 
		//! between subsequent subpasses for maximum efficiency.
		void add_color_transient_attachment(const std::string& name, vk::Format format, uint32_t sample_count = 1)
		{
			check_attachment_name_unique(name);

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

			// Add to global map of string names to attachment descriptions.
			m_attachment_mapping.insert({ name, attachment_description });
		}

		//! Constructs an attachment description for a depth stencil attachment with the specified image format and sample count. 
		//! The initial and final layouts will be set to vk::ImageLayout::eDepthStencilAttachmentOptimal.
		void add_depth_stencil_attachment(const std::string& name, vk::Format format, uint32_t sample_count = 1)
		{
			check_attachment_name_unique(name);

			if (!utils::is_depth_format(format))
			{
				throw std::runtime_error("Attempting to create a depth stencil attachment with an invalid image format");
			}

			vk::AttachmentDescription attachment_description;
			attachment_description.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			attachment_description.format = format;
			attachment_description.initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			attachment_description.loadOp = vk::AttachmentLoadOp::eClear;
			attachment_description.samples = utils::sample_count_to_flags(sample_count);
			attachment_description.stencilLoadOp = utils::is_stencil_format(format) ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare;
			attachment_description.stencilStoreOp = utils::is_stencil_format(format) ? vk::AttachmentStoreOp::eStore : vk::AttachmentStoreOp::eDontCare;
			attachment_description.storeOp = vk::AttachmentStoreOp::eDontCare;

			// Add to global map of string names to attachment descriptions.
			m_attachment_mapping.insert({ name, attachment_description });
		}

		//! Returns `true` if a subpass is currently being recorded into and `false` otherwise.
		bool is_recording() const { return m_is_recording; }

		//! Creates a new subpass entry inside this RenderPassBuilder instance. All subsequent calls to 
		//! `append_attachment_to_subpass(...)` will affect this subpass entry.
		void begin_subpass_record()
		{
			m_is_recording = true;
			m_recorded_subpasses.push_back({});
		}

		//! Associates the attachment with the specified name with the current subpass. You must also specify which 
		//! category the attachment belongs to with respect to the subpass (i.e. AttachmentCategory::CATEGORY_COLOR,
		//! if the specified attachment is going to be used as a color attachment during this subpass). Note that
		//! you must call `begin_subpass_record()` before calling this function.
		void append_attachment_to_subpass(const std::string& name, AttachmentCategory category)
		{
			if (!m_is_recording)
			{
				throw std::runtime_error("The RenderPassBuilder must be in a recording state in order to receive this\
					command - see the `begin_subpass_record()` command for details.");
			}

			m_recorded_subpasses.back().m_categories_to_names_map[category].push_back(name);
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

		//! By default, ending a subpass will append a subpass dependency whose source subpass is the special keyword VK_SUBPASS_EXTERNAL.
		//! If more than one subpass is used, it is up to you to ensure that this subpass dependency is constructed properly.
		void end_subpass_record(const vk::SubpassDependency dependency = create_default_subpass_dependency())
		{
			m_is_recording = false;
			m_recorded_subpass_dependencies.push_back(dependency);
		}

		//! Returns the number of subpasses that have been recorded.
		size_t get_number_of_subpasses() const { return m_recorded_subpasses.size(); }

		//! Returns a vector of all of the user-defined names for render pass attachments.
		std::vector<std::string> get_attachment_names() const
		{
			std::vector<std::string> attachment_names;
			for (const auto& mapping : m_attachment_mapping)
			{
				attachment_names.push_back(mapping.first);
			}
			return attachment_names;
		}

	private:

		struct SubpassRecord
		{
			std::map<AttachmentCategory, std::vector<std::string>> m_categories_to_names_map =
			{
				{ AttachmentCategory::CATEGORY_COLOR,			{} },
				{ AttachmentCategory::CATEGORY_RESOLVE,			{} },
				{ AttachmentCategory::CATEGORY_DEPTH_STENCIL,	{} },
				{ AttachmentCategory::CATEGORY_INPUT,			{} },
				{ AttachmentCategory::CATEGORY_PRESERVE,		{} },
			};

			const std::map<AttachmentCategory, std::vector<std::string>>& get_categories_to_names_map() const
			{
				return m_categories_to_names_map;
			}

			const std::vector<std::string>& get_attachment_names(AttachmentCategory category) const
			{
				return m_categories_to_names_map.at(category);
			}
		};

		//! Returns a vector of all of the recorded subpasses.
		const std::vector<SubpassRecord>& get_subpass_records() const { return m_recorded_subpasses; }

		//! Ensures that the given name does not already exist in the attachment map.
		void check_attachment_name_unique(const std::string& name)
		{
			if (m_attachment_mapping.find(name) != m_attachment_mapping.end())
			{
				throw std::runtime_error("Attachments created with a RenderPassBuilder must have unique names: " + name + " already exists.");
			}
		}

		bool m_is_recording;
		std::vector<SubpassRecord> m_recorded_subpasses;	// TODO: this should also be a map of names -> SubpassRecords.
		std::vector<vk::SubpassDependency> m_recorded_subpass_dependencies;
		std::map<std::string, vk::AttachmentDescription> m_attachment_mapping;

		friend class RenderPass;
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

		//! Factory method for returning a new RenderPassRef from a RenderPassBuilderRef.
		static RenderPassRef create(DeviceWeakRef device, const RenderPassBuilderRef& builder)
		{
			return std::make_shared<RenderPass>(device, builder);
		}

		RenderPass(DeviceWeakRef, const RenderPassBuilderRef& builder);

		~RenderPass();

		vk::RenderPass get_handle() const { return m_render_pass_handle; }

		RenderPassBuilderRef get_render_pass_builder() { return m_render_pass_builder; }

	private:

		DeviceWeakRef m_device;
		RenderPassBuilderRef m_render_pass_builder;
		vk::RenderPass m_render_pass_handle;
	};

} // namespace graphics