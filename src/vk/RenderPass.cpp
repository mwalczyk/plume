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

	std::pair<vk::AttachmentDescription, vk::AttachmentReference> RenderPass::create_color_attachment(vk::Format format, uint32_t attachment)
	{
		// Set up the attachment description.
		vk::AttachmentDescription attachment_description;
		attachment_description.finalLayout = vk::ImageLayout::ePresentSrcKHR;
		attachment_description.format = format;
		attachment_description.initialLayout = vk::ImageLayout::eUndefined;
		attachment_description.loadOp = vk::AttachmentLoadOp::eClear;
		attachment_description.samples = vk::SampleCountFlagBits::e1;
		attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attachment_description.storeOp = vk::AttachmentStoreOp::eStore;

		// Set up the attachment reference.
		vk::AttachmentReference attachment_reference;
		attachment_reference.attachment = attachment;
		attachment_reference.layout = vk::ImageLayout::eColorAttachmentOptimal;

		return { attachment_description, attachment_reference };
	}

	std::pair<vk::AttachmentDescription, vk::AttachmentReference> RenderPass::create_depth_stencil_attachment(vk::Format format, uint32_t attachment, vk::SampleCountFlagBits sample_count)
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
		attachment_description.samples = sample_count;
		attachment_description.stencilLoadOp = utils::is_stencil_format(format) ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eDontCare;
		attachment_description.stencilStoreOp = utils::is_stencil_format(format) ? vk::AttachmentStoreOp::eStore : vk::AttachmentStoreOp::eDontCare;
		attachment_description.storeOp = vk::AttachmentStoreOp::eDontCare;

		// Set up the attachment reference.
		vk::AttachmentReference attachment_reference;
		attachment_reference.attachment = attachment;
		attachment_reference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		return { attachment_description, attachment_reference };
	}

	std::pair<vk::AttachmentDescription, vk::AttachmentReference> RenderPass::create_multisample_attachment(vk::Format format, uint32_t attachment, vk::SampleCountFlagBits sample_count)
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
		attachment_description.samples = sample_count;
		attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attachment_description.storeOp = vk::AttachmentStoreOp::eDontCare;

		// Set up the attachment reference.
		vk::AttachmentReference attachment_reference;
		attachment_reference.attachment = attachment;
		attachment_reference.layout = vk::ImageLayout::eColorAttachmentOptimal;

		return{ attachment_description, attachment_reference };
	}

	vk::SubpassDescription RenderPass::create_subpass_description(const std::vector<vk::AttachmentReference>& color_attachment_references, const vk::AttachmentReference& depth_stencil_attachment_reference)
	{
		vk::SubpassDescription subpass_description;
		subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;	// Currently, only graphics subpasses are supported by Vulkan.
		subpass_description.colorAttachmentCount = static_cast<uint32_t>(color_attachment_references.size());
		subpass_description.pColorAttachments = color_attachment_references.data();
		subpass_description.pDepthStencilAttachment = &depth_stencil_attachment_reference;

		return subpass_description;
	}

	vk::SubpassDependency RenderPass::create_default_subpass_dependency()
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

	RenderPass::Options::Options()
	{
		auto color_attachment = create_color_attachment(vk::Format::eB8G8R8A8Unorm, 0);
		auto depth_attachment = create_depth_stencil_attachment(vk::Format::eD32SfloatS8Uint, 1);

		// Aggregate all attachment references and descriptions.
		m_attachment_descriptions = { color_attachment.first, depth_attachment.first };
		m_attachment_references = { color_attachment.second, depth_attachment.second };

		vk::SubpassDescription subpass_description;
		subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subpass_description.colorAttachmentCount = 1;
		subpass_description.pColorAttachments = &m_attachment_references[0];
		subpass_description.pDepthStencilAttachment = &m_attachment_references[1];
		subpass_description.preserveAttachmentCount = 0;
		subpass_description.pPreserveAttachments = nullptr;
		subpass_description.pResolveAttachments = nullptr;

		m_subpass_descriptions.emplace_back(subpass_description);
		m_subpass_dependencies.emplace_back(create_default_subpass_dependency());
	}

	RenderPass::RenderPass(const DeviceRef& device, const Options& options) :
		m_device(device),
		m_attachment_descriptions(options.m_attachment_descriptions),
		m_attachment_references(options.m_attachment_references),
		m_subpass_descriptions(options.m_subpass_descriptions),
		m_subpass_dependencies(options.m_subpass_dependencies)
	{
		// Create a render pass with the information above.
		vk::RenderPassCreateInfo render_pass_create_info;
		render_pass_create_info.attachmentCount = static_cast<uint32_t>(m_attachment_descriptions.size());
		render_pass_create_info.dependencyCount = static_cast<uint32_t>(m_subpass_dependencies.size());
		render_pass_create_info.pAttachments = m_attachment_descriptions.data();
		render_pass_create_info.pDependencies = m_subpass_dependencies.data();
		render_pass_create_info.pSubpasses = m_subpass_descriptions.data();
		render_pass_create_info.subpassCount = static_cast<uint32_t>(m_subpass_descriptions.size());

		m_render_pass_handle = m_device->get_handle().createRenderPass(render_pass_create_info);
	}

	RenderPass::~RenderPass()
	{
		m_device->get_handle().destroyRenderPass(m_render_pass_handle);
	}

} // namespace graphics
