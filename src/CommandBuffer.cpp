#include "CommandBuffer.h"

namespace vk
{

	CommandBuffer::Options::Options()
	{
		mCommandBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	}

	CommandBuffer::CommandBuffer(const DeviceRef &tDevice, const CommandPoolRef &tCommandPool, const Options &tOptions) :
		mDevice(tDevice),
		mCommandPool(tCommandPool),
		mCommandBufferLevel(tOptions.mCommandBufferLevel)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = mCommandPool->getHandle();
		commandBufferAllocateInfo.level = mCommandBufferLevel;
		commandBufferAllocateInfo.commandBufferCount = 1;

		auto result = vkAllocateCommandBuffers(mDevice->getHandle(), &commandBufferAllocateInfo, &mCommandBufferHandle);
		assert(result == VK_SUCCESS);

		std::cout << "Successfully created command buffer\n";
	}

	CommandBuffer::~CommandBuffer()
	{
		// Command buffers are automatically destroyed when the command pool from which they were allocated are destroyed.
		vkFreeCommandBuffers(mDevice->getHandle(), mCommandPool->getHandle(), 1, &mCommandBufferHandle);
	}

	void CommandBuffer::begin()
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo = {};
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		commandBufferBeginInfo.pInheritanceInfo = nullptr;
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		vkBeginCommandBuffer(mCommandBufferHandle, &commandBufferBeginInfo);
	}

	void CommandBuffer::beginRenderPass(const RenderPassRef &tRenderPass, const FramebufferRef &tFramebuffer)
	{
		VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.framebuffer = tFramebuffer->getHandle();
		renderPassBeginInfo.pClearValues = &clearValue;
		renderPassBeginInfo.pNext = nullptr;
		renderPassBeginInfo.renderArea.extent = { tFramebuffer->getWidth(), tFramebuffer->getHeight() };
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderPass = tRenderPass->getHandle();
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

		vkCmdBeginRenderPass(mCommandBufferHandle, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void CommandBuffer::bindPipeline(const PipelineRef &tPipeline)
	{
		vkCmdBindPipeline(mCommandBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, tPipeline->getHandle());
	}

	void CommandBuffer::updatePushConstantRanges(const PipelineRef &tPipeline, VkShaderStageFlags tStageFlags, uint32_t tOffset, uint32_t tSize, const void* tData)
	{
		vkCmdPushConstants(mCommandBufferHandle, tPipeline->getPipelineLayoutHandle(), tStageFlags, tOffset, tSize, tData);
	}

	void CommandBuffer::updatePushConstantRanges(const PipelineRef &tPipeline, const std::string &tMemberName, const void* tData)
	{
		auto pushConstantsMember = tPipeline->getPushConstantsMember(tMemberName);
		vkCmdPushConstants(mCommandBufferHandle, tPipeline->getPipelineLayoutHandle(), pushConstantsMember.stageFlags, pushConstantsMember.offset, pushConstantsMember.size, tData);
	}

	void CommandBuffer::draw(uint32_t tVertexCount, uint32_t tInstanceCount, uint32_t tFirstVertex, uint32_t tFirstInstance)
	{
		vkCmdDraw(mCommandBufferHandle, tVertexCount, tInstanceCount, tFirstVertex, tFirstInstance);
	}

	void CommandBuffer::endRenderPass()
	{
		vkCmdEndRenderPass(mCommandBufferHandle);
	}

	void CommandBuffer::end()
	{
		auto result = vkEndCommandBuffer(mCommandBufferHandle);
		assert(result == VK_SUCCESS);
	}

} // namespace vk