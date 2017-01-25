#pragma once

#include <memory>

#include "Platform.h"
#include "Device.h"
#include "CommandPool.h"
#include "Framebuffer.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Buffer.h"
#include "Image.h"

namespace vksp
{

	class CommandBuffer;
	using CommandBufferRef = std::shared_ptr<CommandBuffer>;

	class CommandBuffer
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
		void beginRenderPass(const RenderPassRef &tRenderPass, const FramebufferRef &tFramebuffer, const vk::ClearValue &tClearValue = vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }));
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

} // namespace vksp