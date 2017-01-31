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
			
			Options& commandBufferLevel(vk::CommandBufferLevel tCommandBufferLevel) { mCommandBufferLevel = tCommandBufferLevel; return *this; }
		
		private:

			vk::CommandBufferLevel mCommandBufferLevel;

			friend class CommandBuffer;
		};

		//! Factory method for returning a new CommandBufferRef.
		static CommandBufferRef create(const DeviceRef &tDevice, const CommandPoolRef &tCommandPool, const Options &tOptions = Options())
		{
			return std::make_shared<CommandBuffer>(tDevice, tCommandPool, tOptions);
		}

		CommandBuffer(const DeviceRef &tDevice, const CommandPoolRef &tCommandPool, const Options &tOptions = Options());
		~CommandBuffer();

		inline vk::CommandBuffer getHandle() const { return mCommandBufferHandle; };
		inline bool isPrimary() const { return mCommandBufferLevel == vk::CommandBufferLevel::ePrimary; }

		//! Start recording into the command buffer.
		void begin();
		void beginRenderPass(const RenderPassRef &tRenderPass, const FramebufferRef &tFramebuffer, const std::vector<vk::ClearValue> &tClearValues = { vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }) });
		void bindPipeline(const PipelineRef &tPipeline);
		void bindVertexBuffers(const std::vector<BufferRef> &tBuffers);
		void bindIndexBuffer(const BufferRef &tBuffer);
		void updatePushConstantRanges(const PipelineRef &tPipeline, vk::ShaderStageFlags tStageFlags, uint32_t tOffset, uint32_t tSize, const void* tData);
		void updatePushConstantRanges(const PipelineRef &tPipeline, const std::string &tMemberName, const void* tData);
		void draw(uint32_t tVertexCount, uint32_t tInstanceCount, uint32_t tFirstVertex, uint32_t tFirstInstance);
		void drawIndexed(uint32_t tIndexCount, uint32_t tInstanceCount, uint32_t tFirstIndex, uint32_t tVertexOffset, uint32_t tFirstInstance);
		void endRenderPass();

		//! Use an image memory barrier to transition an image from one layout to another.
		void transitionImageLayout(const ImageRef &tImage, vk::ImageLayout tFromLayout, vk::ImageLayout tToLayout);

		//! Stop recording into the command buffer.
		void end();

	private:

		DeviceRef mDevice;
		CommandPoolRef mCommandPool;
		vk::CommandBuffer mCommandBufferHandle;
		vk::CommandBufferLevel mCommandBufferLevel;
	};

} // namespace graphics