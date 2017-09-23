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

#include "Platform.h"
#include "Noncopyable.h"
#include "Device.h"
#include "CommandPool.h"
#include "Framebuffer.h"
#include "Pipeline.h"
#include "Buffer.h"

namespace graphics
{
	
	class CommandBuffer;
	using CommandBufferRef = std::shared_ptr<CommandBuffer>;

	//! Command buffers are objects used to record commands which can be subsequently submitted to a device
	//! queue for execution. When a command buffer begins recording, all state in that command buffer is 
	//! undefined. Unless otherwise specified, and without explicit synchronization, the various commands 
	//! submitted to a queue via command buffers may execute in an arbitrary order relative to one another
	//! and/or concurrently. Therefore, the memory side-effects of any particular command buffer may not 
	//! be directly visible to other commands without explicit barriers.
	//!
	//! For restrictions on using command buffers concurrently, see the notes inside of the command pool class.
	//! 
	//! Command buffers submitted to different queues may execute in parallel or even out of order with
	//! respect to one another. Command buffers submitted to a single queue respect the submission order.
	//! Finally, command buffer execution by the device is asynchronous to host execution. It is the 
	//! application's responsibility to synchronize between the device and host and between different queues.
	//!
	//! Queue submission commands optionally take a list of semaphores upon which to wait before work begins
	//! and a list of semaphores to signal once work has completed. Explicit ordering constraints between
	//! queues can be expressed with semaphores and fences. There are a few implicit ordering constraints
	//! between commands within a command buffer but only covering a subset of execution. Additional explicit
	//! ordering constraints can be expressed with events, pipeline barriers, and subpass dependencies.
	class CommandBuffer : public Noncopyable
	{
	public:

		//! A struct for aggregating the parameters passed to non-indexed drawing commands.
		struct DrawParamsNonIndexed
		{
			DrawParamsNonIndexed(uint32_t vertex_count) :
				m_vertex_count(vertex_count)
			{}

			uint32_t m_vertex_count = 1;	// The number of vertices to draw.
			uint32_t m_instance_count = 1;	// The number of instances to draw.
			uint32_t m_first_vertex = 0;	// The index of the first vertex to draw.
			uint32_t m_first_instance = 0;	// The instance ID of the first instance to draw.
		};

		//! A struct for aggregating the parameters passed to indexed drawing commands.
		struct DrawParamsIndexed
		{
			DrawParamsIndexed(uint32_t index_count) :
				m_index_count(index_count)
			{}

			uint32_t m_index_count = 1;		// The number of vertices to draw.
			uint32_t m_instance_count = 1;	// The number of instances to draw.
			uint32_t m_first_index = 0;		// The base index within the index buffer.
			uint32_t m_vertex_offset = 0;	// The value added to the vertex index before indexing into the vertex buffer.
			uint32_t m_first_instance = 0;	// The instance ID of the first instance to draw.
		};

		//! Factory method for returning a new CommandBufferRef. Allocates a single command buffer from
		//! the specified command pool. The vk::CommandBufferLevel can be either:
		//!
		//! vk::CommandBufferLevel::ePrimary
		//! vk::CommandBufferLevel::eSecondary
		//!
		//! Primary command buffers can execute secondary command buffers. Note that secondary command buffers
		//! are not directly submitted to queues like primary command buffers.
		static CommandBufferRef create(DeviceWeakRef device, const CommandPoolRef& command_pool, vk::CommandBufferLevel command_buffer_level = vk::CommandBufferLevel::ePrimary)
		{
			return std::make_shared<CommandBuffer>(device, command_pool, command_buffer_level);
		}

		CommandBuffer(DeviceWeakRef device, const CommandPoolRef& command_pool, vk::CommandBufferLevel command_buffer_level = vk::CommandBufferLevel::ePrimary);
		
		~CommandBuffer();

		vk::CommandBuffer get_handle() const { return m_command_buffer_handle; };

		//! Determine whether or not this command buffer is a primary or secondary command buffer.
		bool is_primary() const { return m_command_buffer_level == vk::CommandBufferLevel::ePrimary; }

		//! Returns `true` if this command buffer is currently being recorded into (i.e. `begin()` has 
		//! been called).
		bool is_recording() const { return m_is_recording; }

		//! Returns `true` if this command buffer has entered a render pass (i.e. `begin_render_pass()` 
		//! has been called).
		bool is_inside_render_pass() const { return m_is_inside_render_pass; }

		//! Puts the command buffer back into its original state but does not necessarily interact
		//! with the command pool from which it was allocated. Therefore, if the command buffer
		//! dynamically allocates resources from the pool as it grows, it can hang on to those
		//! resources and avoid the cost of reallocation the second and subsequent times it's rebuilt.
		void reset()
		{
			m_is_recording = false;
			m_is_inside_render_pass = false;
			m_command_buffer_handle.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
		}

		//! Start recording into the command buffer. Puts the command buffer into a recording state.
		//! Valid flags are:
		//!
		//! vk::CommandBufferUsageFlagBits::eOneTimeSubmit
		//! vk::CommandBufferUsageFlagBits::eSimultaneousUse
		//!
		//! Note that vk::CommandBufferUsageFlagBits::eRenderPassContinue is only valid for secondary
		//! command buffers.
		void begin(vk::CommandBufferUsageFlags command_buffer_usage_flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		//! Begin recording the commands for a render pass instance. 
		void begin_render_pass(const RenderPassRef& render_pass, const FramebufferRef& framebuffer, const std::vector<vk::ClearValue>& clear_values = { utils::clear_color::black() });

		//! Advance to the current render pass' next subpass.
		void next_subpass();

		//! Set the line width: ignored if the corresponding dynamic state is not part of the active pipeline.
		void set_line_width(float width);

		//! Bind a pipeline for use in subsequent graphics or compute operations.
		void bind_pipeline(const PipelineRef& pipeline);

		//! Binds the specified vertex buffers for use in subsequent draw commands.
		void bind_vertex_buffers(const std::vector<BufferRef>& buffers, uint32_t first_binding = 0);

		//! Binds the specified index buffer for use in subsequent indexed draw commands.
		void bind_index_buffer(const BufferRef& buffer, uint32_t offset = 0, vk::IndexType index_type = vk::IndexType::eUint32);

		//! Update a series of push constants, starting at the specified offset. Note that 
		//! all push constants are undefined at the start of a command buffer.
		template<class T>
		void update_push_constant_ranges(const PipelineRef& pipeline, vk::ShaderStageFlags stage_flags, uint32_t offset, uint32_t size, const T& data)
		{
			// TODO: does this actually work?
			m_command_buffer_handle.pushConstants(pipeline->get_pipeline_layout_handle(), stage_flags, offset, size, &data);
		}

		//! During shader reflection, the pipeline object grabs and stores information about the available push
		//! constants. This let's us refer to a push constant by its string name.
		template<class T>
		void update_push_constant_ranges(const PipelineRef& pipeline, const std::string& name, const T& data)
		{
			auto pushConstantsMember = pipeline->get_push_constants_member(name);

			m_command_buffer_handle.pushConstants(pipeline->get_pipeline_layout_handle(), 
												  pushConstantsMember.stageFlags, 
												  pushConstantsMember.offset, 
												  pushConstantsMember.size, 
												  &data);
		}

		//! Binds the specified descriptor sets.
		void bind_descriptor_sets(const PipelineRef& pipeline, uint32_t first_set, const std::vector<vk::DescriptorSet>& descriptor_sets, const std::vector<uint32_t>& dynamic_offsets = {});

		//! Issue a non-indexed draw command.
		void draw(const DrawParamsNonIndexed& draw_params);

		//! Issue an indexed draw command.
		void draw_indexed(const DrawParamsIndexed& draw_params);

		//! Stop recording the commands for a render pass' final subpass.
		void end_render_pass();

		//! Clear a color image with the specified clear value.
		void clear_color_image(const ImageRef& image, 
							   vk::ClearColorValue clear_value = utils::clear_color::black(), 
							   vk::ImageSubresourceRange image_subresource_range = Image::build_single_layer_subresource());

		//! Clear a depth/stencil image with the specified clear value.
		void clear_depth_image(const ImageRef& image,
							   vk::ClearDepthStencilValue clear_value = utils::clear_depth::depth_one(),
							   vk::ImageSubresourceRange image_subresource_range = Image::build_single_layer_subresource(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil));


		//! Use an image memory barrier to transition an image from one layout to another. This function can also be 
		//! used to transfer ownership from one queue family to another. Note that if `src_queue` and `dst_queue` are
		//! the same, then the image barrier's `srcQueueFamilyIndex` and `dstQueueFamilyIndex` will be set to the special
		//! value VK_QUEUE_FAMILY_IGNORED, meaning that there will be no transfer of ownership.
		void transition_image_layout(const ImageRef& image, 
									 vk::ImageLayout from, 
									 vk::ImageLayout to, 
									 vk::ImageSubresourceRange image_subresource_range = Image::build_single_layer_subresource(),
									 QueueType src_queue = QueueType::GRAPHICS, 
									 QueueType dst_queue = QueueType::GRAPHICS);

		/* 
		 * Common synchronization use cases, expressed as pipeline barriers.
		 *
		 * In a pipeline barrier, the source and destination stage masks describe WHERE the synchronization will happen,
		 * while the source and destination masks in a memory, buffer memory, or image memory barrier describe WHAT
		 * will be happening when the synchronization occurs.
		 *
		 * For more details, see: https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
		 * Also: http://gpuopen.com/vulkan-barriers-explained/
		 *
		 */

		//! Creates a pipeline barrier representing two consecutive compute shader dispatches where the first
		//! writes to a resource, and the second reads from that same resource. This helps avoid a RAW 
		//! (read-after-write) hazard.
		//! 
		//! Examples of this might be:
		//! - Dispatch #1 writes to a storage buffer, dispatch #2 reads from that same storage buffer
		//! - Dispatch #1 writes to a storage image, dispatch #2 reads from that same storage image
		//!   (note that in this case, we can use the same routine for storage images, as storage image
		//!   dependencies are always in vk::ImageLayout::eGeneral - no need for a layout transition)
		//! - Dispatch #1 writes to a storage buffer, dispatch #2 writes to a non-overlapping region
		//!   of the same storage buffer, dispatch #3 reads from both regions
		//! - Dispatch #1 writes to a storage buffer, dispatch #2 writes to a different storage buffer,
		//!   dispatch #3 reads from both storage buffers (note that global memory barriers cover all 
		//!   resources, which is why this and the previous example are both covered by this same function)
		void barrier_compute_write_storage_buffer_compute_read_storage_buffer();

		//! Creates a pipeline barrier representing two consecutive compute shader dispatches where the first
		//! reads from a storage buffer, and the second writes to that same storage buffer. This helps avoid
		//! a WAR (write-after-read) hazard.
		void barrier_compute_read_storage_buffer_compute_write_storage_buffer();

		//! Creates a pipeline barrier representing a compute shader dispatch that writes into a storage
		//! buffer followed by a draw command that consumes that buffer as an index buffer. This avoids
		//! a RAW (read-after-write) hazard.
		void barrier_compute_write_storage_buffer_graphics_read_as_index();

		//! Creates a pipeline barrier representing a compute shader dispatch that writes into a storage
		//! buffer followed by a draw command that consumes that buffer as a draw indirect buffer. This 
		//! avoids a RAW (read-after-write) hazard.
		void barrier_compute_write_storage_buffer_graphics_read_as_draw_indirect();

		//! Creates a pipeline barrier representing a compute shader dispatch that writes into a storage
		//! image followed by a draw command that samples that image in one or more of its subsequent shader
		//! stages. This avoids a RAW (read-after-write) hazard.
		//!
		//! Note that you must also specify which stage of the graphics pipeline you intend to read the
		//! image in. By default, this function assumes that the image will be read in the fragment shader
		//! of the second draw call. To read the image in a different stage, supply the appropriate 
		//! vk::PipelineStageFlagBits. For example, vk::PipelineStageFlagBits::eVertexShader would imply
		//! that the image will be read in the vertex shader stage of the second draw call.
		void barrier_compute_write_storage_image_graphics_read(const ImageRef& image, 
															   vk::PipelineStageFlags read_stage_flags = vk::PipelineStageFlagBits::eFragmentShader,
															   const vk::ImageSubresourceRange& image_subresource_range = Image::build_single_layer_subresource());

		//! Creates a pipeline barrier representing a draw command that writes to a color attachment followed 
		//! by a compute shader dispatch that reads from that image. This avoids a RAW (read-after-write) 
		//! hazard.
		void barrier_graphics_write_color_attachment_compute_read(const ImageRef& image,
																  const vk::ImageSubresourceRange& image_subresource_range = Image::build_single_layer_subresource());
		
		//! Creates a pipeline barrier representing a draw command that writes to a depth attachment followed 
		//! by a compute shader dispatch that reads from that image. This avoids a RAW (read-after-write) 
		//! hazard.
		void barrier_graphics_write_depth_attachment_compute_read(const ImageRef& image,
																  const vk::ImageSubresourceRange& image_subresource_range = Image::build_single_layer_subresource());
		
		//! Creates a pipeline barrier representing a draw command that writes to a depth attachment
		//! followed by another draw command that samples that image in one or more of its subsequent
		//! shader stages. This is useful for shadow map rendering. This avoids a RAW (read-after-write) 
		//! hazard.
		//!
		//! Note that you must also specify which stage of the graphics pipeline you intend to read the
		//! image in. By default, this function assumes that the image will be read in the fragment shader
		//! of the second draw call. To read the image in a different stage, supply the appropriate 
		//! vk::PipelineStageFlagBits. For example, vk::PipelineStageFlagBits::eVertexShader would imply
		//! that the image will be read in the vertex shader stage of the second draw call.
		void barrier_graphics_write_depth_attachment_graphics_read(const ImageRef& image,
																   vk::PipelineStageFlags read_stage_flags = vk::PipelineStageFlagBits::eFragmentShader,
																   const vk::ImageSubresourceRange& image_subresource_range = Image::build_single_layer_subresource());

		//! Creates a pipeline barrier representing a draw command that writes to a color attachment
		//! followed by another draw command that samples that image in one or more of its subsequent
		//! shader stages. This avoids a RAW (read-after-write) hazard. 
		//!
		//! Note that you must also specify which stage of the graphics pipeline you intend to read the
		//! image in. By default, this function assumes that the image will be read in the fragment shader
		//! of the second draw call. To read the image in a different stage, supply the appropriate 
		//! vk::PipelineStageFlagBits. For example, vk::PipelineStageFlagBits::eVertexShader would imply
		//! that the image will be read in the vertex shader stage of the second draw call.
		void barrier_graphics_write_color_attachment_graphics_read(const ImageRef& image,
																   vk::PipelineStageFlags read_stage_flags = vk::PipelineStageFlagBits::eFragmentShader,
																   const vk::ImageSubresourceRange& image_subresource_range = Image::build_single_layer_subresource());

		//! Stop recording into the command buffer. Puts the command buffer into an executable state.
		void end();

	private:

		//! Called before executing any command to verify that the command buffer is in a valid recording state.
		void check_recording_state()
		{
			if (!m_is_recording)
			{
				throw std::runtime_error("Must call `begin()` before attempting to record any command into this CommandBuffer");
			}
		}
		
		//! Called before executing any draw-related command to verify that the command buffer is inside a render pass.
		void check_render_pass_state()
		{
			if (!m_is_inside_render_pass)
			{
				throw std::runtime_error("Must call `begin_render_pass()` before attempting to record any draw-related command into this CommandBuffer");
			}
		}

		DeviceWeakRef m_device;
		CommandPoolRef m_command_pool;
		vk::CommandBuffer m_command_buffer_handle;
		vk::CommandBufferLevel m_command_buffer_level;
		bool m_is_recording;
		bool m_is_inside_render_pass;
	};

	class ScopedRecord : Noncopyable
	{
	public:

		// On construction, begin recording into the specified command buffer
		ScopedRecord(const CommandBufferRef& command_buffer) :
			m_command_buffer(command_buffer)
		{
			m_command_buffer->begin();
		}

		// On destruction, end recording into the specified command buffer
		~ScopedRecord()
		{
			m_command_buffer->end();
		}

	private:

		CommandBufferRef m_command_buffer;
	};

} // namespace graphics