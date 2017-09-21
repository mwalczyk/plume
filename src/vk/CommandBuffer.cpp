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

	CommandBuffer::CommandBuffer(DeviceWeakRef device, const CommandPoolRef& command_pool, vk::CommandBufferLevel command_buffer_level) :
		m_device(device),
		m_command_pool(command_pool),
		m_command_buffer_level(command_buffer_level),
		m_is_recording(false),
		m_is_inside_render_pass(false)
	{
		DeviceRef device_shared = m_device.lock();

		vk::CommandBufferAllocateInfo command_buffer_allocate_info;
		command_buffer_allocate_info.commandPool = m_command_pool->get_handle();
		command_buffer_allocate_info.level = m_command_buffer_level;
		command_buffer_allocate_info.commandBufferCount = 1;

		m_command_buffer_handle = device_shared->get_handle().allocateCommandBuffers(command_buffer_allocate_info)[0];
	}

	CommandBuffer::~CommandBuffer()
	{
		DeviceRef device_shared = m_device.lock();

		// Command buffers are automatically destroyed when the command pool from which they were allocated are destroyed.
		device_shared->get_handle().freeCommandBuffers(m_command_pool->get_handle(), m_command_buffer_handle);
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
		DeviceRef device_shared = m_device.lock();

		auto range = device_shared->get_physical_device_properties().limits.lineWidthRange;
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

	void CommandBuffer::bind_descriptor_sets(const PipelineRef& pipeline, uint32_t first_set, const std::vector<vk::DescriptorSet>& descriptor_sets, const std::vector<uint32_t>& dynamic_offsets)
	{
		m_command_buffer_handle.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline->get_pipeline_layout_handle(), first_set, static_cast<uint32_t>(descriptor_sets.size()), descriptor_sets.data(), static_cast<uint32_t>(dynamic_offsets.size()), dynamic_offsets.data());
	}

	void CommandBuffer::draw(const DrawParamsNonIndexed& draw_params)
	{
		m_command_buffer_handle.draw(draw_params.m_vertex_count, 
									 draw_params.m_instance_count, 
									 draw_params.m_first_vertex, 
									 draw_params.m_first_instance);
	}

	void CommandBuffer::draw_indexed(const DrawParamsIndexed& draw_params)
	{
		m_command_buffer_handle.drawIndexed(draw_params.m_index_count, 
											draw_params.m_instance_count,
											draw_params.m_first_index,
											draw_params.m_vertex_offset,
											draw_params.m_first_instance);
	}

	void CommandBuffer::end_render_pass()
	{
		if (m_is_inside_render_pass)
		{
			m_command_buffer_handle.endRenderPass();
			m_is_inside_render_pass = false;
		}
	}

	void CommandBuffer::clear_color_image(const ImageRef& image, vk::ClearColorValue clear_value, vk::ImageSubresourceRange image_subresource_range)
	{
		if (utils::is_depth_format(image->get_format()))
		{
			throw std::runtime_error("Attempting to clear a depth/stencil image with `clear_color_image`");
		}
		m_command_buffer_handle.clearColorImage(image->get_handle(), image->get_current_layout(), clear_value, image_subresource_range);
	}

	void CommandBuffer::clear_depth_image(const ImageRef& image, vk::ClearDepthStencilValue clear_value, vk::ImageSubresourceRange image_subresource_range)
	{
		if (!utils::is_depth_format(image->get_format()))
		{
			throw std::runtime_error("Attempting to clear a color image with `clear_depth_image`");
		}
		m_command_buffer_handle.clearDepthStencilImage(image->get_handle(), image->get_current_layout(), clear_value, image_subresource_range);
	}

	void CommandBuffer::transition_image_layout(const ImageRef& image, 
												vk::ImageLayout from, 
												vk::ImageLayout to, 
												QueueType src_queue, 
												QueueType dst_queue)
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

		DeviceRef device_shared = m_device.lock();

		// TODO: the image might be layered
		image_memory_barrier.dstQueueFamilyIndex = (src_queue == dst_queue) ? VK_QUEUE_FAMILY_IGNORED : device_shared->get_queue_family_index(src_queue);
		image_memory_barrier.image = image->get_handle();
		image_memory_barrier.newLayout = to;
		image_memory_barrier.oldLayout = from;
		image_memory_barrier.srcQueueFamilyIndex = (src_queue == dst_queue) ? VK_QUEUE_FAMILY_IGNORED : device_shared->get_queue_family_index(src_queue);
		image_memory_barrier.subresourceRange.aspectMask = utils::format_to_aspect_mask(image->get_format());
		image_memory_barrier.subresourceRange.baseArrayLayer = 0;
		image_memory_barrier.subresourceRange.baseMipLevel = 0;
		image_memory_barrier.subresourceRange.layerCount = 1;
		image_memory_barrier.subresourceRange.levelCount = 1;

		// This class is a friend of the image class - store its new layout.
		image->m_current_layout = to;

		// The `srcStageMask` and `dstStageMask` specify which pipeline stages wrote to the resource
		// last and which stages will read from the resource next, respectively. That is, they specify
		// the source and destination for the data flow represented by the barrier. The stage 
		// vk::PipelineStageFlagBits::eTopOfPipe is considered to be hit as soon as the device starts
		// processing the command.
		//
		// The `dependencyFlags` parameter specifies a set of flags that describe how the dependency 
		// represented by the barrier affects the resources referenced by the barrier. 
		m_command_buffer_handle.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe, {}, {}, {}, image_memory_barrier);
	}

	void CommandBuffer::barrier_compute_write_storage_buffer_compute_read_storage_buffer()
	{
		static vk::MemoryBarrier memory_barrier;
		memory_barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		m_command_buffer_handle.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,		// Source stage mask
												vk::PipelineStageFlagBits::eComputeShader,		// Destination stage mask
												{},												// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
												memory_barrier, {}, {});						// Memory barriers, buffer memory barriers, image memory barriers
	}

	void CommandBuffer::barrier_compute_read_storage_buffer_compute_write_storage_buffer()
	{
		// WAR hazards don't need a memory barrier between them - a simple execution barrier is sufficient.

		m_command_buffer_handle.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,		// Source stage mask
												vk::PipelineStageFlagBits::eComputeShader,		// Destination stage mask
												{},												// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
												{}, {}, {});									// Memory barriers, buffer memory barriers, image memory barriers
	}

	void CommandBuffer::barrier_compute_write_storage_buffer_graphics_read_as_index()
	{
		static vk::MemoryBarrier memory_barrier;
		memory_barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		memory_barrier.dstAccessMask = vk::AccessFlagBits::eIndexRead;

		m_command_buffer_handle.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,		// Source stage mask
												vk::PipelineStageFlagBits::eVertexInput,		// Destination stage mask
												{},												// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
												memory_barrier, {}, {});						// Memory barriers, buffer memory barriers, image memory barriers
	}

	void CommandBuffer::barrier_compute_write_storage_buffer_graphics_read_as_draw_indirect()
	{
		static vk::MemoryBarrier memory_barrier;
		memory_barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		memory_barrier.dstAccessMask = vk::AccessFlagBits::eIndirectCommandRead;

		m_command_buffer_handle.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,		// Source stage mask
												vk::PipelineStageFlagBits::eDrawIndirect,		// Destination stage mask
												{},												// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
												memory_barrier, {}, {});						// Memory barriers, buffer memory barriers, image memory barriers
	}

	void CommandBuffer::barrier_compute_write_storage_image_graphics_read(const ImageRef& image, 
																		  vk::PipelineStageFlags read_stage_flags,
																		  const vk::ImageSubresourceRange& image_subresource_range)
	{
		vk::ImageMemoryBarrier image_memory_barrier;
		image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
		image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		image_memory_barrier.oldLayout = vk::ImageLayout::eGeneral;
		image_memory_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		image_memory_barrier.image = image->get_handle();
		image_memory_barrier.subresourceRange = image_subresource_range;

		image->m_current_layout = image_memory_barrier.newLayout;

		m_command_buffer_handle.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,		// Source stage mask
												read_stage_flags,								// Destination stage mask (fragment shader, by default)
												{},												// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
												{}, {}, image_memory_barrier);					// Memory barriers, buffer memory barriers, image memory barriers
	}

	void CommandBuffer::barrier_graphics_write_color_attachment_compute_read(const ImageRef& image, const vk::ImageSubresourceRange& image_subresource_range)
	{
		vk::ImageMemoryBarrier image_memory_barrier;
		image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		image_memory_barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
		image_memory_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		image_memory_barrier.image = image->get_handle();
		image_memory_barrier.subresourceRange = image_subresource_range;

		image->m_current_layout = image_memory_barrier.newLayout;

		m_command_buffer_handle.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,	// Source stage mask
												vk::PipelineStageFlagBits::eComputeShader,			// Destination stage mask
												{},													// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
												{}, {}, image_memory_barrier);						// Memory barriers, buffer memory barriers, image memory barriers
	}

	void CommandBuffer::barrier_graphics_write_depth_attachment_compute_read(const ImageRef& image, const vk::ImageSubresourceRange& image_subresource_range)
	{
		vk::ImageMemoryBarrier image_memory_barrier;
		image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		image_memory_barrier.oldLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		image_memory_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		image_memory_barrier.image = image->get_handle();
		image_memory_barrier.subresourceRange = image_subresource_range;

		image->m_current_layout = image_memory_barrier.newLayout;

		m_command_buffer_handle.pipelineBarrier(vk::PipelineStageFlagBits::eEarlyFragmentTests |
												vk::PipelineStageFlagBits::eLateFragmentTests,		// Source stage mask
												vk::PipelineStageFlagBits::eComputeShader,			// Destination stage mask
												{},													// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
												{}, {}, image_memory_barrier);						// Memory barriers, buffer memory barriers, image memory barriers
	}

	void CommandBuffer::barrier_graphics_write_depth_attachment_graphics_read(const ImageRef& image, 
																			  vk::PipelineStageFlags read_stage_flags, 
																			  const vk::ImageSubresourceRange& image_subresource_range)
	{
		vk::ImageMemoryBarrier image_memory_barrier;
		image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		image_memory_barrier.oldLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		image_memory_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		image_memory_barrier.image = image->get_handle();
		image_memory_barrier.subresourceRange = image_subresource_range;

		image->m_current_layout = image_memory_barrier.newLayout;

		m_command_buffer_handle.pipelineBarrier(vk::PipelineStageFlagBits::eEarlyFragmentTests |
												vk::PipelineStageFlagBits::eLateFragmentTests,		// Source stage mask
												read_stage_flags,									// Destination stage mask (fragment shader, by default)
												{},													// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
												{}, {}, image_memory_barrier);						// Memory barriers, buffer memory barriers, image memory barriers
	}

	void CommandBuffer::barrier_graphics_write_color_attachment_graphics_read(const ImageRef& image, 
																			  vk::PipelineStageFlags read_stage_flags, 
																			  const vk::ImageSubresourceRange& image_subresource_range)
	{
		vk::ImageMemoryBarrier image_memory_barrier;
		image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
		image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		image_memory_barrier.oldLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		image_memory_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		image_memory_barrier.image = image->get_handle();
		image_memory_barrier.subresourceRange = image_subresource_range;

		image->m_current_layout = image_memory_barrier.newLayout;

		m_command_buffer_handle.pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,  // Source stage mask
												read_stage_flags,									// Destination stage mask (fragment shader, by default)
												{},													// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
												{}, {}, image_memory_barrier);						// Memory barriers, buffer memory barriers, image memory barriers
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