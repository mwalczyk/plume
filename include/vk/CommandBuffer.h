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
#include "RenderPass.h"
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
		static CommandBufferRef create(const DeviceRef& device, const CommandPoolRef& command_pool, vk::CommandBufferLevel command_buffer_level = vk::CommandBufferLevel::ePrimary)
		{
			return std::make_shared<CommandBuffer>(device, command_pool, command_buffer_level);
		}

		CommandBuffer(const DeviceRef& device, const CommandPoolRef& command_pool, vk::CommandBufferLevel command_buffer_level = vk::CommandBufferLevel::ePrimary);
		~CommandBuffer();

		inline vk::CommandBuffer get_handle() const { return m_command_buffer_handle; };

		//! Determine whether or not this command buffer is a primary or secondary command buffer.
		inline bool is_primary() const { return m_command_buffer_level == vk::CommandBufferLevel::ePrimary; }

		//! Returns `true` if this command buffer is currently being recorded into (i.e. `begin()` has 
		//! been called).
		inline bool is_recording() const { return m_is_recording; }

		//! Returns `true` if this command buffer has entered a render pass (i.e. `begin_render_pass()` 
		//! has been called).
		inline bool is_inside_render_pass() const { return m_is_inside_render_pass; }


		//! Puts the command buffer back into its original state but does not necessarily interact
		//! with the command pool from which it was allocated. Therefore, if the command buffer
		//! dynamically allocates resources from the pool as it grows, it can hang on to those
		//! resources and avoid the cost of reallocation the second and subsequent times it's rebuilt.
		inline void reset()
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
		void begin_render_pass(const RenderPassRef& render_pass, const FramebufferRef& framebuffer, const std::vector<vk::ClearValue>& clear_values = { vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }) });

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
			m_command_buffer_handle.pushConstants(pipeline->get_pipeline_layout_handle(), stage_flags, offset, size, &data);
		}

		//! During shader reflection, the pipeline object grabs and stores information about the available push
		//! constants. This let's us refer to a push constant by its string name.
		template<class T>
		void update_push_constant_ranges(const PipelineRef& pipeline, const std::string& name, const T& data)
		{
			auto pushConstantsMember = pipeline->get_push_constants_member(name);

			m_command_buffer_handle.pushConstants(pipeline->get_pipeline_layout_handle(), pushConstantsMember.stageFlags, pushConstantsMember.offset, pushConstantsMember.size, &data);
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
			vk::ClearColorValue clear_value = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f }, 
			vk::ImageSubresourceRange image_subresource_range = Image::build_single_layer_subresource());

		//! Clear a depth/stencil image with the specified clear value.
		void clear_depth_image(const ImageRef& image,
			vk::ClearDepthStencilValue clear_value = { 1.0f, 0 },
			vk::ImageSubresourceRange image_subresource_range = Image::build_single_layer_subresource(vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil));


		//! Use an image memory barrier to transition an image from one layout to another.
		void transition_image_layout(const ImageRef& image, vk::ImageLayout from, vk::ImageLayout to);

		//! Stop recording into the command buffer. Puts the command buffer into an executable state.
		void end();

	private:

		DeviceRef m_device;
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