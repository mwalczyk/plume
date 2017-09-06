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
			all_attachment_names.push_back(mapping.first);	// The colloquial name of the attachment (i.e. "color_attachment_0")
			all_attachment_descs.push_back(mapping.second);	// The vk::AttachmentDescription struct describing this attachment

			// For example, the two vectors above might look like:
			// "color_0", "color_1", "depth", ...
			// with a vk::AttachmentDescription for each
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
		for (const auto& subpass_record : builder->m_recorded_subpasses)
		{
			//static const std::vector<RenderPassBuilder::AttachmentCategory

			// Color
			auto attachment_names = subpass_record.get_attachment_names(RenderPassBuilder::AttachmentCategory::CATEGORY_COLOR);
			for (const auto& name : attachment_names)
			{
				// Find the index corresponding to this attachment.
				uint32_t index = find(all_attachment_names.begin(), all_attachment_names.end(), name) - all_attachment_names.begin();
				
				vk::AttachmentReference attachment_reference = { index, vk::ImageLayout::eColorAttachmentOptimal };
				all_attachment_color_refs[subpass_index].push_back(attachment_reference);
			}

			// Resolve
			attachment_names = subpass_record.get_attachment_names(RenderPassBuilder::AttachmentCategory::CATEGORY_RESOLVE);
			for (const auto& name : attachment_names)
			{
				// Find the index corresponding to this attachment.
				uint32_t index = find(all_attachment_names.begin(), all_attachment_names.end(), name) - all_attachment_names.begin();

				vk::AttachmentReference attachment_reference = { index, vk::ImageLayout::eColorAttachmentOptimal };
				all_attachment_resolve_refs[subpass_index].push_back(attachment_reference);
			}

			// Depth stencil
			attachment_names = subpass_record.get_attachment_names(RenderPassBuilder::AttachmentCategory::CATEGORY_DEPTH_STENCIL);
			for (const auto& name : attachment_names)
			{
				// Find the index corresponding to this attachment.
				uint32_t index = find(all_attachment_names.begin(), all_attachment_names.end(), name) - all_attachment_names.begin();

				vk::AttachmentReference attachment_reference = { index, vk::ImageLayout::eDepthStencilAttachmentOptimal };
				all_attachment_depth_refs[subpass_index].push_back(attachment_reference);
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
