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

	CommandBuffer::Options::Options()
	{
		m_command_buffer_level = vk::CommandBufferLevel::ePrimary;
	}

	CommandBuffer::CommandBuffer(const DeviceRef& device, const CommandPoolRef& command_pool, const Options& options) :
		m_device(device),
		m_command_pool(command_pool),
		m_command_buffer_level(options.m_command_buffer_level)
	{
		vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.commandPool = m_command_pool->get_handle();
		commandBufferAllocateInfo.level = m_command_buffer_level;
		commandBufferAllocateInfo.commandBufferCount = 1;

		m_command_buffer_handle = m_device->getHandle().allocateCommandBuffers(commandBufferAllocateInfo)[0];

	}

	CommandBuffer::~CommandBuffer()
	{
		// Command buffers are automatically destroyed when the command pool from which they were allocated are destroyed.
		m_device->getHandle().freeCommandBuffers(m_command_pool->get_handle(), m_command_buffer_handle);
	}

	void CommandBuffer::begin()
	{
		vk::CommandBufferBeginInfo commandBuffer_begin_info;
		commandBuffer_begin_info.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		commandBuffer_begin_info.pInheritanceInfo = nullptr;

		m_command_buffer_handle.begin(commandBuffer_begin_info);
	}

	void CommandBuffer::begin_render_pass(const RenderPassRef& render_pass, const FramebufferRef& framebuffer, const std::vector<vk::ClearValue>& clear_values)
	{
		vk::RenderPassBeginInfo render_pass_begin_info;
		render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
		render_pass_begin_info.framebuffer = framebuffer->get_handle();
		render_pass_begin_info.pClearValues = clear_values.data();
		render_pass_begin_info.renderArea.extent = { framebuffer->get_width(), framebuffer->get_height() };
		render_pass_begin_info.renderArea.offset = { 0, 0 };
		render_pass_begin_info.renderPass = render_pass->get_handle();

		m_command_buffer_handle.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
	}

	void CommandBuffer::bind_pipeline(const PipelineRef& pipeline)
	{
		m_command_buffer_handle.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->getHandle());
	}

	void CommandBuffer::bind_vertex_buffers(const std::vector<BufferRef>& buffers)
	{
		std::vector<vk::Buffer> buffer_handles(buffers.size());
		std::transform(buffers.begin(), buffers.end(), buffer_handles.begin(), [](const BufferRef& buffer) { return buffer->get_handle(); } );
		std::vector<vk::DeviceSize> offsets(buffers.size(), 0);

		uint32_t first_binding = 0;
		uint32_t binding_count = static_cast<uint32_t>(buffers.size());

		m_command_buffer_handle.bindVertexBuffers(first_binding, buffer_handles, offsets);
	}

	void CommandBuffer::bind_index_buffer(const BufferRef& buffer)
	{
		m_command_buffer_handle.bindIndexBuffer(buffer->get_handle(), 0, vk::IndexType::eUint32);
	}

	void CommandBuffer::update_push_constant_ranges(const PipelineRef& pipeline, vk::ShaderStageFlags stage_flags, uint32_t offset, uint32_t size, const void* data)
	{
		m_command_buffer_handle.pushConstants(pipeline->getPipelineLayoutHandle(), stage_flags, offset, size, data);
	}

	void CommandBuffer::update_push_constant_ranges(const PipelineRef& pipeline, const std::string& name, const void* data)
	{
		auto pushConstantsMember = pipeline->getPushConstantsMember(name);

		m_command_buffer_handle.pushConstants(pipeline->getPipelineLayoutHandle(), pushConstantsMember.stageFlags, pushConstantsMember.offset, pushConstantsMember.size, data);
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
		m_command_buffer_handle.endRenderPass();
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
		else 
		{
			throw std::invalid_argument("Unsupported layout transition");
		}

		image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.image = image->get_handle();
		image_memory_barrier.newLayout = to;
		image_memory_barrier.oldLayout = from;
		image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_memory_barrier.subresourceRange.aspectMask = ImageBase::format_to_aspect_mask(image->get_format());
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
		m_command_buffer_handle.end();
	}

} // namespace graphics