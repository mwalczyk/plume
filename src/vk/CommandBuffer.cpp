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

namespace plume
{

	namespace graphics
	{

		CommandBuffer::CommandBuffer(const Device& device, const CommandPool& command_pool, vk::CommandBufferLevel command_buffer_level) :

			m_device_ptr(&device),
			m_command_pool_ptr(&command_pool),
			m_command_buffer_level(command_buffer_level),
			m_is_recording(false),
			m_is_inside_render_pass(false)
		{
			vk::CommandBufferAllocateInfo command_buffer_allocate_info;
			command_buffer_allocate_info.commandPool = m_command_pool_ptr->get_handle();
			command_buffer_allocate_info.level = m_command_buffer_level;
			command_buffer_allocate_info.commandBufferCount = 1;

			m_command_buffer_handle = std::move(m_device_ptr->get_handle().allocateCommandBuffersUnique(command_buffer_allocate_info)[0]);
		}

		void CommandBuffer::begin(vk::CommandBufferUsageFlags command_buffer_usage_flags)
		{
			m_is_recording = true;

			vk::CommandBufferBeginInfo command_buffer_begin_info;
			command_buffer_begin_info.flags = command_buffer_usage_flags;
			command_buffer_begin_info.pInheritanceInfo = nullptr;

			get_handle().begin(command_buffer_begin_info);
		}

		void CommandBuffer::begin_render_pass(const RenderPass& render_pass, const Framebuffer& framebuffer, const std::vector<vk::ClearValue>& clear_values)
		{
			check_recording_state();

			if (m_is_inside_render_pass)
			{
				throw std::runtime_error("This command buffer is already inside of a render pass");
			}

			m_is_inside_render_pass = true;

			vk::RenderPassBeginInfo render_pass_begin_info;
			render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
			render_pass_begin_info.framebuffer = framebuffer.get_handle();
			render_pass_begin_info.pClearValues = clear_values.data();
			render_pass_begin_info.renderArea.extent = { framebuffer.get_width(), framebuffer.get_height() };
			render_pass_begin_info.renderArea.offset = { 0, 0 };
			render_pass_begin_info.renderPass = render_pass.get_handle();

			get_handle().beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
		}

		void CommandBuffer::next_subpass()
		{
			check_recording_state();
			check_render_pass_state();

			get_handle().nextSubpass(vk::SubpassContents::eInline);
		}

		void CommandBuffer::set_line_width(float width)
		{
			check_recording_state();

			auto range = m_device_ptr->get_physical_device_properties().limits.lineWidthRange;
			float remapped = std::min(range[1], std::max(range[0], width));
			get_handle().setLineWidth(remapped);
		}

		void CommandBuffer::bind_pipeline(const Pipeline& pipeline)
		{
			check_recording_state();

			get_handle().bindPipeline(pipeline.get_pipeline_bind_point(), pipeline.get_handle());
		}

		void CommandBuffer::bind_vertex_buffer(const Buffer& buffer, uint32_t binding, vk::DeviceSize offset)
		{
			check_recording_state();

			if (!(buffer.get_buffer_usage_flags() & vk::BufferUsageFlagBits::eVertexBuffer))
			{
				throw std::runtime_error("One or more of the buffer objects passed to `bind_vertex_buffers()` was not created with the\
									  vk::BufferUsageFlagBits::eVertexBuffer bit set");
			}

			get_handle().bindVertexBuffers(binding, buffer.get_handle(), offset);
		}

		void CommandBuffer::bind_index_buffer(const Buffer& buffer, uint32_t offset, vk::IndexType index_type)
		{
			check_recording_state();

			if (!(buffer.get_buffer_usage_flags() & vk::BufferUsageFlagBits::eIndexBuffer))
			{
				throw std::runtime_error("The buffer object passed to `bind_index_buffer()` was not created with the\
									  vk::BufferUsageFlagBits::eIndexBuffer bit set");
			}

			get_handle().bindIndexBuffer(buffer.get_handle(), offset, index_type);
		}

		void CommandBuffer::bind_descriptor_sets(const Pipeline& pipeline, uint32_t first_set, const std::vector<vk::DescriptorSet>& descriptor_sets, const std::vector<uint32_t>& dynamic_offsets)
		{
			check_recording_state();

			get_handle().bindDescriptorSets(pipeline.get_pipeline_bind_point(),
				pipeline.get_pipeline_layout_handle(),
				first_set,
				static_cast<uint32_t>(descriptor_sets.size()),
				descriptor_sets.data(),
				static_cast<uint32_t>(dynamic_offsets.size()),
				dynamic_offsets.data());
		}

		void CommandBuffer::draw(const DrawParamsNonIndexed& draw_params)
		{
			check_recording_state();
			check_render_pass_state();

			get_handle().draw(draw_params.m_vertex_count,
				draw_params.m_instance_count,
				draw_params.m_first_vertex,
				draw_params.m_first_instance);
		}

		void CommandBuffer::draw_indexed(const DrawParamsIndexed& draw_params)
		{
			check_recording_state();
			check_render_pass_state();

			get_handle().drawIndexed(draw_params.m_index_count,
				draw_params.m_instance_count,
				draw_params.m_first_index,
				draw_params.m_vertex_offset,
				draw_params.m_first_instance);
		}

		void CommandBuffer::end_render_pass()
		{
			check_recording_state();
			check_render_pass_state();

			get_handle().endRenderPass();
			m_is_inside_render_pass = false;
		}

		void CommandBuffer::clear_color_image(const Image& image, vk::ClearColorValue clear_value, vk::ImageSubresourceRange image_subresource_range)
		{
			check_recording_state();

			if (utils::is_depth_format(image.get_format()))
			{
				throw std::runtime_error("Attempting to clear a depth/stencil image with `clear_color_image()`");
			}
			get_handle().clearColorImage(image.get_handle(), image.get_current_layout(), clear_value, image_subresource_range);
		}

		void CommandBuffer::clear_depth_image(const Image& image, vk::ClearDepthStencilValue clear_value, vk::ImageSubresourceRange image_subresource_range)
		{
			check_recording_state();

			if (!utils::is_depth_format(image.get_format()))
			{
				throw std::runtime_error("Attempting to clear a color image with `clear_depth_image()`");
			}
			get_handle().clearDepthStencilImage(image.get_handle(), image.get_current_layout(), clear_value, image_subresource_range);
		}

		void CommandBuffer::transition_image_layout(const Image& image,
			vk::ImageLayout from,
			vk::ImageLayout to,
			vk::ImageSubresourceRange image_subresource_range,
			QueueType src_queue,
			QueueType dst_queue)
		{
			check_recording_state();

			vk::ImageMemoryBarrier image_memory_barrier;

			// Based on the starting and ending layout of this image, select the appropriate access masks. See Sascha Willems' 
			// examples for more details: https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.cpp#L94

		   /***********************************************************************************
			*
			* Source
			*
			***********************************************************************************/
			switch (from)
			{
			case vk::ImageLayout::eUndefined:
				image_memory_barrier.srcAccessMask = {};
				break;
			case vk::ImageLayout::eGeneral:
				image_memory_barrier.srcAccessMask = {};
				break;
			case vk::ImageLayout::eColorAttachmentOptimal:
				if (!(image.get_image_usage_flags() & vk::ImageUsageFlagBits::eColorAttachment))
				{
					throw std::runtime_error("Attempting to create an image memory barrier with `oldLayout` vk::ImageLayout::eColorAttachmentOptimal,\
										  but this image was not created with usage flags vk::ImageUsageFlagBits::eColorAttachment");
				}

				image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
				break;
			case vk::ImageLayout::eDepthStencilAttachmentOptimal:
				if (!(image.get_image_usage_flags() & vk::ImageUsageFlagBits::eDepthStencilAttachment))
				{
					throw std::runtime_error("Attempting to create an image memory barrier with `oldLayout` vk::ImageLayout::eDepthStencilAttachmentOptimal,\
										  but this image was not created with usage flags vk::ImageUsageFlagBits::eDepthStencilAttachment");
				}

				image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
				break;
			case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
				if (!(image.get_image_usage_flags() & vk::ImageUsageFlagBits::eDepthStencilAttachment))
				{
					throw std::runtime_error("Attempting to create an image memory barrier with `oldLayout` vk::ImageLayout::eDepthStencilReadOnlyOptimal,\
										  but this image was not created with usage flags vk::ImageUsageFlagBits::eDepthStencilAttachment");
				}

				image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead;
				break;
			case vk::ImageLayout::eShaderReadOnlyOptimal:
				if (!(image.get_image_usage_flags() & vk::ImageUsageFlagBits::eSampled) ||
					!(image.get_image_usage_flags() & vk::ImageUsageFlagBits::eInputAttachment))
				{
					throw std::runtime_error("Attempting to create an image memory barrier with `oldLayout` vk::ImageLayout::eShaderReadOnlyOptimal,\
										  but this image was not created with usage flags vk::ImageUsageFlagBits::eSampled or vk::ImageUsageFlagBits::eInputAttachment");
				}

				image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
				break;
			case vk::ImageLayout::eTransferSrcOptimal:
				if (!(image.get_image_usage_flags() & vk::ImageUsageFlagBits::eTransferSrc))
				{
					throw std::runtime_error("Attempting to create an image memory barrier with `oldLayout` vk::ImageLayout::eTransferSrcOptimal,\
										  but this image was not created with usage flags vk::ImageUsageFlagBits::eTransferSrc");
				}

				image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
				break;
			case vk::ImageLayout::eTransferDstOptimal:
				if (!(image.get_image_usage_flags() & vk::ImageUsageFlagBits::eTransferDst))
				{
					throw std::runtime_error("Attempting to create an image memory barrier with `oldLayout` vk::ImageLayout::eTransferDstOptimal,\
										  but this image was not created with usage flags vk::ImageUsageFlagBits::eTransferDst");
				}

				image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
				break;
			case vk::ImageLayout::ePreinitialized:
				image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
				break;
			case vk::ImageLayout::ePresentSrcKHR:		// TODO: this should only be valid for swapchain images.
			case vk::ImageLayout::eSharedPresentKHR:	// TODO: ...
			default:
				break;
			}


			/***********************************************************************************
			 *
			 * Destination
			 *
			 ***********************************************************************************/
			switch (to)
			{
			case vk::ImageLayout::eUndefined:
				throw std::runtime_error("Attempting to create an image memory barrier with `newLayout` vk::ImageLayout::eUndefined, which\
									  can only be used as `oldLayout`");
				break;
			case vk::ImageLayout::eGeneral:
				image_memory_barrier.dstAccessMask = {};
				break;
			case vk::ImageLayout::eColorAttachmentOptimal:
				if (!(image.get_image_usage_flags() & vk::ImageUsageFlagBits::eColorAttachment))
				{
					throw std::runtime_error("Attempting to create an image memory barrier with `newLayout` vk::ImageLayout::eColorAttachmentOptimal,\
										  but this image was not created with usage flags vk::ImageUsageFlagBits::eColorAttachment");
				}

				image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
				break;
			case vk::ImageLayout::eDepthStencilAttachmentOptimal:
				if (!(image.get_image_usage_flags() & vk::ImageUsageFlagBits::eDepthStencilAttachment))
				{
					throw std::runtime_error("Attempting to create an image memory barrier with `newLayout` vk::ImageLayout::eDepthStencilAttachmentOptimal,\
										  but this image was not created with usage flags vk::ImageUsageFlagBits::eDepthStencilAttachment");
				}

				image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
				break;
			case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
				if (!(image.get_image_usage_flags() & vk::ImageUsageFlagBits::eDepthStencilAttachment))
				{
					throw std::runtime_error("Attempting to create an image memory barrier with `newLayout` vk::ImageLayout::eDepthStencilReadOnlyOptimal,\
										  but this image was not created with usage flags vk::ImageUsageFlagBits::eDepthStencilAttachment");
				}

				image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead;
				break;
			case vk::ImageLayout::eShaderReadOnlyOptimal:
				if (!(image.get_image_usage_flags() & vk::ImageUsageFlagBits::eSampled) ||
					!(image.get_image_usage_flags() & vk::ImageUsageFlagBits::eInputAttachment))
				{
					throw std::runtime_error("Attempting to create an image memory barrier with `newLayout` vk::ImageLayout::eShaderReadOnlyOptimal,\
										  but this image was not created with usage flags vk::ImageUsageFlagBits::eSampled or vk::ImageUsageFlagBits::eInputAttachment");
				}

				image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
				break;
			case vk::ImageLayout::eTransferSrcOptimal:
				if (!(image.get_image_usage_flags() & vk::ImageUsageFlagBits::eTransferSrc))
				{
					throw std::runtime_error("Attempting to create an image memory barrier with `newLayout` vk::ImageLayout::eTransferSrcOptimal,\
										  but this image was not created with usage flags vk::ImageUsageFlagBits::eTransferSrc");
				}

				image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
				break;
			case vk::ImageLayout::eTransferDstOptimal:
				if (!(image.get_image_usage_flags() & vk::ImageUsageFlagBits::eTransferDst))
				{
					throw std::runtime_error("Attempting to create an image memory barrier with `newLayout` vk::ImageLayout::eTransferDstOptimal,\
										  but this image was not created with usage flags vk::ImageUsageFlagBits::eTransferDst");
				}

				image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
				break;
			case vk::ImageLayout::ePreinitialized:
				throw std::runtime_error("Attempting to create an image memory barrier with `newLayout` vk::ImageLayout::ePreinitialized, which\
									  can only be used as `oldLayout`");
				break;
			case vk::ImageLayout::ePresentSrcKHR:		// TODO: this should only be valid for swapchain images.
			case vk::ImageLayout::eSharedPresentKHR:	// TODO: ...
			default:
				break;
			}

			image_memory_barrier.dstQueueFamilyIndex = (src_queue == dst_queue) ? VK_QUEUE_FAMILY_IGNORED : m_device_ptr->get_queue_family_index(src_queue);
			image_memory_barrier.image = image.get_handle();
			image_memory_barrier.newLayout = to;
			image_memory_barrier.oldLayout = from;
			image_memory_barrier.srcQueueFamilyIndex = (src_queue == dst_queue) ? VK_QUEUE_FAMILY_IGNORED : m_device_ptr->get_queue_family_index(src_queue);
			image_memory_barrier.subresourceRange = image_subresource_range;

			// For now, infer the subresource range's aspect mask from the parent image's format. We might not
			// want to do this in the future.
			image_memory_barrier.subresourceRange.aspectMask = utils::format_to_aspect_mask(image.get_format());

			// This class is a friend of the image class - store its new layout.
			image.m_current_layout = to;

			// The `srcStageMask` and `dstStageMask` specify which pipeline stages wrote to the resource
			// last and which stages will read from the resource next, respectively. That is, they specify
			// the source and destination for the data flow represented by the barrier. The stage 
			// vk::PipelineStageFlagBits::eTopOfPipe is considered to be hit as soon as the device starts
			// processing the command.
			//
			// The `dependencyFlags` parameter specifies a set of flags that describe how the dependency 
			// represented by the barrier affects the resources referenced by the barrier. 
			//
			// TODO: the `srcStageMask` and `dstStageMask` parameters should probably not be top of pipe.
			get_handle().pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
				vk::PipelineStageFlagBits::eTopOfPipe,
				{}, {}, {}, image_memory_barrier);
		}

		void CommandBuffer::barrier_compute_write_storage_buffer_compute_read_storage_buffer()
		{
			check_recording_state();

			static vk::MemoryBarrier memory_barrier;
			memory_barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
			memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			get_handle().pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,		// Source stage mask
				vk::PipelineStageFlagBits::eComputeShader,		// Destination stage mask
				{},											// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
				memory_barrier, {}, {});						// Memory barriers, buffer memory barriers, image memory barriers
		}

		void CommandBuffer::barrier_compute_read_storage_buffer_compute_write_storage_buffer()
		{
			check_recording_state();

			// WAR hazards don't need a memory barrier between them - a simple execution barrier is sufficient.

			get_handle().pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,		// Source stage mask
				vk::PipelineStageFlagBits::eComputeShader,		// Destination stage mask
				{},											// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
				{}, {}, {});									// Memory barriers, buffer memory barriers, image memory barriers
		}

		void CommandBuffer::barrier_compute_write_storage_buffer_graphics_read_as_index()
		{
			check_recording_state();

			static vk::MemoryBarrier memory_barrier;
			memory_barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
			memory_barrier.dstAccessMask = vk::AccessFlagBits::eIndexRead;

			get_handle().pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,		// Source stage mask
				vk::PipelineStageFlagBits::eVertexInput,		// Destination stage mask
				{},											// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
				memory_barrier, {}, {});						// Memory barriers, buffer memory barriers, image memory barriers
		}

		void CommandBuffer::barrier_compute_write_storage_buffer_graphics_read_as_draw_indirect()
		{
			check_recording_state();

			static vk::MemoryBarrier memory_barrier;
			memory_barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
			memory_barrier.dstAccessMask = vk::AccessFlagBits::eIndirectCommandRead;

			get_handle().pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,		// Source stage mask
				vk::PipelineStageFlagBits::eDrawIndirect,		// Destination stage mask
				{},											// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
				memory_barrier, {}, {});						// Memory barriers, buffer memory barriers, image memory barriers
		}

		void CommandBuffer::barrier_compute_write_storage_image_graphics_read(const Image& image,
			vk::PipelineStageFlags read_stage_flags,
			const vk::ImageSubresourceRange& image_subresource_range)
		{
			check_recording_state();

			vk::ImageMemoryBarrier image_memory_barrier;
			image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
			image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			image_memory_barrier.oldLayout = vk::ImageLayout::eGeneral;
			image_memory_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			image_memory_barrier.image = image.get_handle();
			image_memory_barrier.subresourceRange = image_subresource_range;

			image.m_current_layout = image_memory_barrier.newLayout;

			get_handle().pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,		// Source stage mask
				read_stage_flags,								// Destination stage mask (fragment shader, by default)
				{},											// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
				{}, {}, image_memory_barrier);					// Memory barriers, buffer memory barriers, image memory barriers
		}

		void CommandBuffer::barrier_graphics_write_color_attachment_compute_read(const Image& image, const vk::ImageSubresourceRange& image_subresource_range)
		{
			check_recording_state();

			vk::ImageMemoryBarrier image_memory_barrier;
			image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
			image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			image_memory_barrier.oldLayout = vk::ImageLayout::eColorAttachmentOptimal;
			image_memory_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			image_memory_barrier.image = image.get_handle();
			image_memory_barrier.subresourceRange = image_subresource_range;

			image.m_current_layout = image_memory_barrier.newLayout;

			get_handle().pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput,	// Source stage mask
				vk::PipelineStageFlagBits::eComputeShader,			// Destination stage mask
				{},												// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
				{}, {}, image_memory_barrier);						// Memory barriers, buffer memory barriers, image memory barriers
		}

		void CommandBuffer::barrier_graphics_write_depth_attachment_compute_read(const Image& image, const vk::ImageSubresourceRange& image_subresource_range)
		{
			check_recording_state();

			vk::ImageMemoryBarrier image_memory_barrier;
			image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			image_memory_barrier.oldLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			image_memory_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			image_memory_barrier.image = image.get_handle();
			image_memory_barrier.subresourceRange = image_subresource_range;

			image.m_current_layout = image_memory_barrier.newLayout;

			get_handle().pipelineBarrier(vk::PipelineStageFlagBits::eEarlyFragmentTests |
				vk::PipelineStageFlagBits::eLateFragmentTests,		// Source stage mask
				vk::PipelineStageFlagBits::eComputeShader,			// Destination stage mask
				{},												// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
				{}, {}, image_memory_barrier);						// Memory barriers, buffer memory barriers, image memory barriers
		}

		void CommandBuffer::barrier_graphics_write_depth_attachment_graphics_read(const Image& image,
			vk::PipelineStageFlags read_stage_flags,
			const vk::ImageSubresourceRange& image_subresource_range)
		{
			check_recording_state();

			vk::ImageMemoryBarrier image_memory_barrier;
			image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			image_memory_barrier.oldLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			image_memory_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			image_memory_barrier.image = image.get_handle();
			image_memory_barrier.subresourceRange = image_subresource_range;

			image.m_current_layout = image_memory_barrier.newLayout;

			get_handle().pipelineBarrier(vk::PipelineStageFlagBits::eEarlyFragmentTests |
				vk::PipelineStageFlagBits::eLateFragmentTests,		// Source stage mask
				read_stage_flags,									// Destination stage mask (fragment shader, by default)
				{},												// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
				{}, {}, image_memory_barrier);						// Memory barriers, buffer memory barriers, image memory barriers
		}

		void CommandBuffer::barrier_graphics_write_color_attachment_graphics_read(const Image& image,
			vk::PipelineStageFlags read_stage_flags,
			const vk::ImageSubresourceRange& image_subresource_range)
		{
			check_recording_state();

			vk::ImageMemoryBarrier image_memory_barrier;
			image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
			image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			image_memory_barrier.oldLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			image_memory_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			image_memory_barrier.image = image.get_handle();
			image_memory_barrier.subresourceRange = image_subresource_range;

			image.m_current_layout = image_memory_barrier.newLayout;

			get_handle().pipelineBarrier(vk::PipelineStageFlagBits::eColorAttachmentOutput, // Source stage mask
				read_stage_flags,									// Destination stage mask (fragment shader, by default)
				{},												// Dependency flags (can only be vk::DependencyFlagBits::eByRegion)
				{}, {}, image_memory_barrier);						// Memory barriers, buffer memory barriers, image memory barriers
		}

		void CommandBuffer::end()
		{
			check_recording_state();

			get_handle().end();
			m_is_recording = false;
		}

	} // namespace graphics

} // namespace plume