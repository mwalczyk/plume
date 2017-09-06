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

	RenderPass::RenderPass(DeviceWeakRef device, const RenderPassBuilder& builder) :
		m_device(device)
	{
		DeviceRef device_shared = m_device.lock();

		std::vector<vk::AttachmentDescription> all_attachment_descs;
		std::vector<vk::SubpassDescription> all_subpass_descs;
		std::vector<vk::SubpassDependency> all_subpass_deps;

		for (const auto& subpass_record : builder.get_subpass_records())
		{
			// TODO: this isn't correct and will not respect the order of color / resolve
			// attachments with respect to one another. For example, if we first add a color
			// attachment at attachment point #1 then a resolve attachment at attachment point
			// #2, this would break.
			all_attachment_descs.insert(std::end(all_attachment_descs),
				std::begin(subpass_record.m_resolve_attachment_descs),
				std::end(subpass_record.m_resolve_attachment_descs));

			all_attachment_descs.insert(std::end(all_attachment_descs), 
										std::begin(subpass_record.m_color_attachment_descs), 
										std::end(subpass_record.m_color_attachment_descs));

			all_attachment_descs.push_back(subpass_record.m_depth_stencil_attachment_desc);

			all_subpass_descs.push_back(subpass_record.build_subpass_description());

			all_subpass_deps.push_back(subpass_record.get_subpass_dependency());
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
