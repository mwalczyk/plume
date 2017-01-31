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
#include "Image.h"

namespace graphics
{

	class CommandBuffer;
	using CommandBufferRef = std::shared_ptr<CommandBuffer>;

	class CommandBuffer : public Noncopyable
	{
	public:

		class Options
		{
		public:

			Options();
			
			Options& command_buffer_level(vk::CommandBufferLevel command_buffer_level) { m_command_buffer_level = command_buffer_level; return *this; }
		
		private:

			vk::CommandBufferLevel m_command_buffer_level;

			friend class CommandBuffer;
		};

		//! Factory method for returning a new CommandBufferRef.
		static CommandBufferRef create(const DeviceRef& device, const CommandPoolRef& command_pool, const Options& options = Options())
		{
			return std::make_shared<CommandBuffer>(device, command_pool, options);
		}

		CommandBuffer(const DeviceRef& device, const CommandPoolRef& command_pool, const Options& options = Options());
		~CommandBuffer();

		inline vk::CommandBuffer get_handle() const { return m_command_buffer_handle; };
		inline bool is_primary() const { return m_command_buffer_level == vk::CommandBufferLevel::ePrimary; }

		//! Start recording into the command buffer.
		void begin();
		void begin_render_pass(const RenderPassRef& render_pass, const FramebufferRef& framebuffer, const std::vector<vk::ClearValue>& clear_values = { vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }) });
		void bind_pipeline(const PipelineRef& pipeline);
		void bind_vertex_buffers(const std::vector<BufferRef>& buffers);
		void bind_index_buffer(const BufferRef& buffer);
		void update_push_constant_ranges(const PipelineRef& pipeline, vk::ShaderStageFlags stage_flags, uint32_t offset, uint32_t size, const void* data);
		void update_push_constant_ranges(const PipelineRef& pipeline, const std::string& name, const void* data);
		void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
		void draw_indexed(uint32_t tIndexCount, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance);
		void end_render_pass();

		//! Use an image memory barrier to transition an image from one layout to another.
		void transition_image_layout(const ImageRef& image, vk::ImageLayout from, vk::ImageLayout to);

		//! Stop recording into the command buffer.
		void end();

	private:

		DeviceRef m_device;
		CommandPoolRef m_command_pool;
		vk::CommandBuffer m_command_buffer_handle;
		vk::CommandBufferLevel m_command_buffer_level;
	};

} // namespace graphics