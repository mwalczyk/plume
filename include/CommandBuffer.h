#pragma once

#include <memory>

#include "Platform.h"
#include "Device.h"
#include "CommandPool.h"
#include "Framebuffer.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "Buffer.h"

namespace vk
{

	class CommandBuffer;
	using CommandBufferRef = std::shared_ptr<CommandBuffer>;

	class CommandBuffer
	{

	public:

		struct Options
		{
			Options();

			Options& commandBufferLevel(VkCommandBufferLevel tCommandBufferLevel) { mCommandBufferLevel = tCommandBufferLevel; return *this; }

			VkCommandBufferLevel mCommandBufferLevel;
		};

		//! Factory method for returning a new CommandBufferRef.
		static CommandBufferRef create(const DeviceRef &tDevice, const CommandPoolRef &tCommandPool, const Options &tOptions = Options())
		{
			return std::make_shared<CommandBuffer>(tDevice, tCommandPool, tOptions);
		}

		CommandBuffer(const DeviceRef &tDevice, const CommandPoolRef &tCommandPool, const Options &tOptions = Options());
		~CommandBuffer();

		inline VkCommandBuffer getHandle() const { return mCommandBufferHandle; };
		
		//! Start recording into the command buffer.
		void begin();
		void beginRenderPass(const RenderPassRef &tRenderPass, const FramebufferRef &tFramebuffer);
		void bindPipeline(const PipelineRef &tPipeline);
		void bindVertexBuffers(const std::vector<BufferRef> &tBuffers);
		void updatePushConstantRanges(const PipelineRef &tPipeline, VkShaderStageFlags tStageFlags, uint32_t tOffset, uint32_t tSize, const void* tData);
		void updatePushConstantRanges(const PipelineRef &tPipeline, const std::string &tMemberName, const void* tData);
		void draw(uint32_t tVertexCount, uint32_t tInstanceCount, uint32_t tFirstVertex, uint32_t tFirstInstance);
		void endRenderPass();

		//! Stop recording into the command buffer.
		void end();

	private:

		VkCommandBuffer mCommandBufferHandle;

		DeviceRef mDevice;
		CommandPoolRef mCommandPool;

		VkCommandBufferLevel mCommandBufferLevel;

	};

} // namespace vk