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

#include "RenderPass.h"

namespace graphics
{

	namespace
	{

		const std::map<AttachmentCategory, vk::ImageLayout> default_image_layouts = 
		{
			{ AttachmentCategory::CATEGORY_COLOR,			vk::ImageLayout::eColorAttachmentOptimal },
			{ AttachmentCategory::CATEGORY_RESOLVE,			vk::ImageLayout::eColorAttachmentOptimal },
			{ AttachmentCategory::CATEGORY_DEPTH_STENCIL,	vk::ImageLayout::eDepthStencilAttachmentOptimal },
			{ AttachmentCategory::CATEGORY_INPUT,			vk::ImageLayout::eColorAttachmentOptimal },
			{ AttachmentCategory::CATEGORY_PRESERVE,		vk::ImageLayout::eColorAttachmentOptimal }
		};

	} // anonymous

	RenderPass::RenderPass(DeviceWeakRef device, const RenderPassBuilderRef& builder) :
		m_device(device),
		m_render_pass_builder(builder)
	{
		DeviceRef device_shared = m_device.lock();

		// First, separate the names and attachment descriptions into two independent
		// vectors - we need to do this so that we can pass the vk::AttachmentDescription
		// structs to the render pass constructor.
		std::vector<std::string>				all_attachment_names;
		std::vector<vk::AttachmentDescription>	all_attachment_descs;
		for (const auto& mapping : builder->m_attachment_mapping)
		{
			all_attachment_names.push_back(mapping.first);	// The colloquial name of the attachment (i.e. "color_a")
			all_attachment_descs.push_back(mapping.second);	// The vk::AttachmentDescription struct describing this attachment
		}

		// A vector of vectors - each "outer" vector represents a particular subpass, while each
		// "inner" vector represents the attachment references of a particular type belonging to 
		// that subpass. 
		std::vector<std::vector<vk::AttachmentReference>>	all_attachment_color_refs(builder->m_recorded_subpasses.size());
		std::vector<std::vector<vk::AttachmentReference>>	all_attachment_resolve_refs(builder->m_recorded_subpasses.size());
		std::vector<std::vector<vk::AttachmentReference>>	all_attachment_depth_refs(builder->m_recorded_subpasses.size());
		std::vector<std::vector<vk::AttachmentReference>>	all_attachment_input_refs(builder->m_recorded_subpasses.size());
		std::vector<std::vector<vk::AttachmentReference>>	all_attachment_preserve_refs(builder->m_recorded_subpasses.size());

		std::vector<vk::SubpassDescription>					all_subpass_descs(builder->m_recorded_subpasses.size());
		std::vector<vk::SubpassDependency>					all_subpass_deps = builder->m_recorded_subpass_dependencies;

		size_t subpass_index = 0;

		// Find and create all attachment references corresponding to this subpass.
		for (auto& subpass_record : builder->m_recorded_subpasses)
		{
			// Iterate over all of the possible attachment categories for this particular subpass.
			// Each category is associated with a set of attachment names (strings). This mapping is
			// maintained internally by the SubpassRecord struct. We use the static map above to allow
			// for easy iteration over this set. We also associate each cateogry with a default image
			// layout, since the vk::AttachmentReference struct requires this. 
			//
			// TODO: are these layouts correct?
			for (auto mapping : default_image_layouts)
			{
				AttachmentCategory category = mapping.first;
				vk::ImageLayout default_image_layout = mapping.second;

				// Iterate over all of the names associated with this attachment category, for example:
				//
				//			CATEGORY_COLOR:		{ "color_a", "color_b", "color_c", ... }
				//			CATEGORY_RESOLVE:	{ "ms_resolve_a", "ms_resolve_b", "ms_resolve_c", ... }
				//			...
				for (const auto& name : subpass_record.get_attachment_names(category))
				{
					// Find the index corresponding to the attachment with this name. For example:
					//
					//		{ "color_a", "color_b", "color_c", ... }
					//
					// the attachment "color_c" would have the index 2.
					uint32_t index = find(all_attachment_names.begin(), all_attachment_names.end(), name) - all_attachment_names.begin();

					vk::AttachmentReference attachment_reference = { index, default_image_layout };

					// Finally, based on the attachment category, add the newly created attachment
					// reference to the appropriate global list that will be used to construct this
					// subpass description.
					switch (category)
					{
					case AttachmentCategory::CATEGORY_COLOR:
						all_attachment_color_refs[subpass_index].push_back(attachment_reference);
						break;
					case AttachmentCategory::CATEGORY_RESOLVE:
						all_attachment_resolve_refs[subpass_index].push_back(attachment_reference);
						break;
					case AttachmentCategory::CATEGORY_DEPTH_STENCIL:
						all_attachment_depth_refs[subpass_index].push_back(attachment_reference);
						break;
					case AttachmentCategory::CATEGORY_INPUT:
						all_attachment_input_refs[subpass_index].push_back(attachment_reference);
						break;
					case AttachmentCategory::CATEGORY_PRESERVE:
					default:
						all_attachment_preserve_refs[subpass_index].push_back(attachment_reference);
						break;
					}
					
				}
			}

			// Create the subpass description based on the attachment references created above.
			vk::SubpassDescription subpass_description = {};
			subpass_description.colorAttachmentCount = static_cast<uint32_t>(all_attachment_color_refs[subpass_index].size());
			subpass_description.inputAttachmentCount = 0;
			subpass_description.preserveAttachmentCount = 0;

			subpass_description.pColorAttachments = all_attachment_color_refs[subpass_index].data();
			subpass_description.pResolveAttachments = all_attachment_resolve_refs[subpass_index].data();
			subpass_description.pDepthStencilAttachment = all_attachment_depth_refs[subpass_index].data();
			subpass_description.pInputAttachments = nullptr;
			subpass_description.pPreserveAttachments = nullptr;
			
			subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

			// Finally, add the subpass description to the global list.
			all_subpass_descs[subpass_index] = subpass_description;

			// Increment the subpass index.
			subpass_index++;
		}

		// Create a render pass with the information above.
		vk::RenderPassCreateInfo render_pass_create_info;
		render_pass_create_info.attachmentCount = static_cast<uint32_t>(all_attachment_descs.size());
		render_pass_create_info.dependencyCount = static_cast<uint32_t>(all_subpass_deps.size());
		render_pass_create_info.pAttachments = all_attachment_descs.data();
		render_pass_create_info.pDependencies = all_subpass_deps.data();
		render_pass_create_info.pSubpasses = all_subpass_descs.data();
		render_pass_create_info.subpassCount = static_cast<uint32_t>(all_subpass_descs.size());

		m_render_pass_handle = device_shared->get_handle().createRenderPass(render_pass_create_info);
	}

	RenderPass::~RenderPass()
	{
		DeviceRef device_shared = m_device.lock();

		device_shared->get_handle().destroyRenderPass(m_render_pass_handle);
	}

} // namespace graphics
