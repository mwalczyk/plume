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

#include "CommandBuffer.h"

namespace graphics
{

	CommandBuffer::CommandBuffer(const DeviceRef& device, const CommandPoolRef& command_pool, vk::CommandBufferLevel command_buffer_level) :
		m_device(device),
		m_command_pool(command_pool),
		m_command_buffer_level(command_buffer_level),
		m_is_recording(false),
		m_is_inside_render_pass(false)
	{
		vk::CommandBufferAllocateInfo command_buffer_allocate_info;
		command_buffer_allocate_info.commandPool = m_command_pool->get_handle();
		command_buffer_allocate_info.level = m_command_buffer_level;
		command_buffer_allocate_info.commandBufferCount = 1;

		m_command_buffer_handle = m_device->get_handle().allocateCommandBuffers(command_buffer_allocate_info)[0];
	}

	CommandBuffer::~CommandBuffer()
	{
		// Command buffers are automatically destroyed when the command pool from which they were allocated are destroyed.
		m_device->get_handle().freeCommandBuffers(m_command_pool->get_handle(), m_command_buffer_handle);
	}

	void CommandBuffer::begin(vk::CommandBufferUsageFlags command_buffer_usage_flags)
	{
		m_is_recording = true;

		vk::CommandBufferBeginInfo command_buffer_begin_info;
		command_buffer_begin_info.flags = command_buffer_usage_flags;
		command_buffer_begin_info.pInheritanceInfo = nullptr;

		m_command_buffer_handle.begin(command_buffer_begin_info);
	}

	void CommandBuffer::begin_render_pass(const RenderPassRef& render_pass, const FramebufferRef& framebuffer, const std::vector<vk::ClearValue>& clear_values)
	{
		if (m_is_inside_render_pass)
		{
			throw std::runtime_error("This command buffer is already inside of a render pass");
		}

		m_is_inside_render_pass = true;

		vk::RenderPassBeginInfo render_pass_begin_info;
		render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
		render_pass_begin_info.framebuffer = framebuffer->get_handle();
		render_pass_begin_info.pClearValues = clear_values.data();
		render_pass_begin_info.renderArea.extent = { framebuffer->get_width(), framebuffer->get_height() };
		render_pass_begin_info.renderArea.offset = { 0, 0 };
		render_pass_begin_info.renderPass = render_pass->get_handle();

		m_command_buffer_handle.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
	}

	void CommandBuffer::next_subpass()
	{
		m_command_buffer_handle.nextSubpass(vk::SubpassContents::eInline);
	}

	void CommandBuffer::set_line_width(float width)
	{
		auto range = m_device->get_physical_device_properties().limits.lineWidthRange;
		float remapped = std::min(range[1], std::max(range[0], width));
		m_command_buffer_handle.setLineWidth(remapped);
	}

	void CommandBuffer::bind_pipeline(const PipelineRef& pipeline)
	{
		m_command_buffer_handle.bindPipeline(pipeline->get_pipeline_bind_point(), pipeline->get_handle());
	}

	void CommandBuffer::bind_vertex_buffers(const std::vector<BufferRef>& buffers, uint32_t first_binding)
	{
		// Gather all of the buffer handles.
		std::vector<vk::Buffer> buffer_handles(buffers.size());
		std::transform(buffers.begin(), buffers.end(), buffer_handles.begin(), [](const BufferRef& buffer) { return buffer->get_handle(); } );

		// TODO: for now, set all buffer offsets to 0.
		std::vector<vk::DeviceSize> offsets(buffers.size(), 0);

		m_command_buffer_handle.bindVertexBuffers(first_binding, buffer_handles, offsets);
	}

	void CommandBuffer::bind_index_buffer(const BufferRef& buffer, uint32_t offset, vk::IndexType index_type)
	{
		m_command_buffer_handle.bindIndexBuffer(buffer->get_handle(), offset, index_type);
	}

	void CommandBuffer::update_push_constant_ranges(const PipelineRef& pipeline, vk::ShaderStageFlags stage_flags, uint32_t offset, uint32_t size, const void* data)
	{
		// TODO: is it possible to use a template with copy here instead of a pointer to the data?

		m_command_buffer_handle.pushConstants(pipeline->get_pipeline_layout_handle(), stage_flags, offset, size, data);
	}

	void CommandBuffer::update_push_constant_ranges(const PipelineRef& pipeline, const std::string& name, const void* data)
	{
		// TODO: is it possible to use a template with copy here instead of a pointer to the data?

		auto pushConstantsMember = pipeline->get_push_constants_member(name);

		m_command_buffer_handle.pushConstants(pipeline->get_pipeline_layout_handle(), pushConstantsMember.stageFlags, pushConstantsMember.offset, pushConstantsMember.size, data);
	}

	void CommandBuffer::bind_descriptor_sets(const PipelineRef& pipeline, uint32_t first_set, const std::vector<vk::DescriptorSet>& descriptor_sets, const std::vector<uint32_t>& dynamic_offsets)
	{
		m_command_buffer_handle.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline->get_pipeline_layout_handle(), first_set, static_cast<uint32_t>(descriptor_sets.size()), descriptor_sets.data(), static_cast<uint32_t>(dynamic_offsets.size()), dynamic_offsets.data());
	}

	void CommandBuffer::draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
	{
		m_command_buffer_handle.draw(vertex_count, instance_count, first_vertex, first_instance);
	}

	void CommandBuffer::draw_indexed(uint32_t tIndexCount, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance)
	{
		m_command_buffer_handle.drawIndexed(tIndexCount, instance_count, first_index, vertex_offset, first_instance);
	}

	void CommandBuffer::end_render_pass()
	{
		if (m_is_inside_render_pass)
		{
			m_command_buffer_handle.endRenderPass();
			m_is_inside_render_pass = false;
		}
	}

	void CommandBuffer::transition_image_layout(const ImageRef& image, vk::ImageLayout from, vk::ImageLayout to)
	{
		vk::ImageMemoryBarrier image_memory_barrier;

		// Based on the starting and ending layout of this image, select the appropriate access masks.
		if (from == vk::ImageLayout::ePreinitialized && to == vk::ImageLayout::eTransferSrcOptimal)
		{
			image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
			image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
		}
		else if (from == vk::ImageLayout::ePreinitialized && to == vk::ImageLayout::eTransferDstOptimal)
		{
			image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
			image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
		}
		else if (from == vk::ImageLayout::eTransferDstOptimal && to == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		}
		else if (from == vk::ImageLayout::ePreinitialized && to == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
			image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		}
		else if (from == vk::ImageLayout::eUndefined && to == vk::ImageLayout::eDepthStencilAttachmentOptimal)
		{
			image_memory_barrier.srcAccessMask = {};
			image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		}
		else if (from == vk::ImageLayout::eUndefined && to == vk::ImageLayout::eGeneral)
		{
			// TODO: what are these suppose to be?
			image_memory_barrier.srcAccessMask = {};
			image_memory_barrier.dstAccessMask = {};
		}
		else 
		{
			throw std::invalid_argument("Unsupported layout transition");
		}

		// TODO: the image might be layered
		image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.image = image->get_handle();
		image_memory_barrier.newLayout = to;
		image_memory_barrier.oldLayout = from;
		image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.subresourceRange.aspectMask = utils::format_to_aspect_mask(image->get_format());
		image_memory_barrier.subresourceRange.baseArrayLayer = 0;
		image_memory_barrier.subresourceRange.baseMipLevel = 0;
		image_memory_barrier.subresourceRange.layerCount = 1;
		image_memory_barrier.subresourceRange.levelCount = 1;

		// This class is a friend of the image class - store its new layout.
		image->m_current_layout = to;

		m_command_buffer_handle.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe, {}, {}, {}, image_memory_barrier);
	}

	void CommandBuffer::end()
	{
		if (m_is_recording)
		{
			m_command_buffer_handle.end();
			m_is_recording = false;
		}
	}

} // namespace graphics