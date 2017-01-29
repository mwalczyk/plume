#include "CommandBuffer.h"

namespace graphics
{

	CommandBuffer::Options::Options()
	{
		mCommandBufferLevel = vk::CommandBufferLevel::ePrimary;
	}

	CommandBuffer::CommandBuffer(const DeviceRef &tDevice, const CommandPoolRef &tCommandPool, const Options &tOptions) :
		mDevice(tDevice),
		mCommandPool(tCommandPool),
		mCommandBufferLevel(tOptions.mCommandBufferLevel)
	{
		vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
		commandBufferAllocateInfo.commandPool = mCommandPool->getHandle();
		commandBufferAllocateInfo.level = mCommandBufferLevel;
		commandBufferAllocateInfo.commandBufferCount = 1;

		mCommandBufferHandle = mDevice->getHandle().allocateCommandBuffers(commandBufferAllocateInfo)[0];

	}

	CommandBuffer::~CommandBuffer()
	{
		// Command buffers are automatically destroyed when the command pool from which they were allocated are destroyed.
		mDevice->getHandle().freeCommandBuffers(mCommandPool->getHandle(), mCommandBufferHandle);
	}

	void CommandBuffer::begin()
	{
		vk::CommandBufferBeginInfo commandBufferBeginInfo;
		commandBufferBeginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
		commandBufferBeginInfo.pInheritanceInfo = nullptr;

		mCommandBufferHandle.begin(commandBufferBeginInfo);
	}

	void CommandBuffer::beginRenderPass(const RenderPassRef &tRenderPass, const FramebufferRef &tFramebuffer, const std::vector<vk::ClearValue> &tClearValues)
	{
		vk::RenderPassBeginInfo renderPassBeginInfo;
		renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(tClearValues.size());
		renderPassBeginInfo.framebuffer = tFramebuffer->getHandle();
		renderPassBeginInfo.pClearValues = tClearValues.data();
		renderPassBeginInfo.renderArea.extent = { tFramebuffer->getWidth(), tFramebuffer->getHeight() };
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderPass = tRenderPass->getHandle();

		mCommandBufferHandle.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
	}

	void CommandBuffer::bindPipeline(const PipelineRef &tPipeline)
	{
		mCommandBufferHandle.bindPipeline(vk::PipelineBindPoint::eGraphics, tPipeline->getHandle());
	}

	void CommandBuffer::bindVertexBuffers(const std::vector<BufferRef> &tBuffers)
	{
		std::vector<vk::Buffer> bufferHandles(tBuffers.size());
		std::transform(tBuffers.begin(), tBuffers.end(), bufferHandles.begin(), [](const BufferRef &tBuffer) { return tBuffer->getHandle(); } );
		std::vector<vk::DeviceSize> offsets(tBuffers.size(), 0);

		uint32_t firstBinding = 0;
		uint32_t bindingCount = static_cast<uint32_t>(tBuffers.size());

		mCommandBufferHandle.bindVertexBuffers(firstBinding, bufferHandles, offsets);
	}

	void CommandBuffer::bindIndexBuffer(const BufferRef &tBuffer)
	{
		mCommandBufferHandle.bindIndexBuffer(tBuffer->getHandle(), 0, vk::IndexType::eUint32);
	}

	void CommandBuffer::updatePushConstantRanges(const PipelineRef &tPipeline, vk::ShaderStageFlags tStageFlags, uint32_t tOffset, uint32_t tSize, const void* tData)
	{
		mCommandBufferHandle.pushConstants(tPipeline->getPipelineLayoutHandle(), tStageFlags, tOffset, tSize, tData);
	}

	void CommandBuffer::updatePushConstantRanges(const PipelineRef &tPipeline, const std::string &tMemberName, const void* tData)
	{
		auto pushConstantsMember = tPipeline->getPushConstantsMember(tMemberName);

		mCommandBufferHandle.pushConstants(tPipeline->getPipelineLayoutHandle(), pushConstantsMember.stageFlags, pushConstantsMember.offset, pushConstantsMember.size, tData);
	}

	void CommandBuffer::draw(uint32_t tVertexCount, uint32_t tInstanceCount, uint32_t tFirstVertex, uint32_t tFirstInstance)
	{
		mCommandBufferHandle.draw(tVertexCount, tInstanceCount, tFirstVertex, tFirstInstance);
	}

	void CommandBuffer::drawIndexed(uint32_t tIndexCount, uint32_t tInstanceCount, uint32_t tFirstIndex, uint32_t tVertexOffset, uint32_t tFirstInstance)
	{
		mCommandBufferHandle.drawIndexed(tIndexCount, tInstanceCount, tFirstIndex, tVertexOffset, tFirstInstance);
	}

	void CommandBuffer::endRenderPass()
	{
		mCommandBufferHandle.endRenderPass();
	}

	void CommandBuffer::transitionImageLayout(const ImageRef &tImage, vk::ImageLayout tFromLayout, vk::ImageLayout tToLayout)
	{
		vk::ImageMemoryBarrier imageMemoryBarrier;

		// Based on the starting and ending layout of this image, select the appropriate access masks.
		if (tFromLayout == vk::ImageLayout::ePreinitialized && tToLayout == vk::ImageLayout::eTransferSrcOptimal)
		{
			imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
			imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
		}
		else if (tFromLayout == vk::ImageLayout::ePreinitialized && tToLayout == vk::ImageLayout::eTransferDstOptimal)
		{
			imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
			imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
		}
		else if (tFromLayout == vk::ImageLayout::eTransferDstOptimal && tToLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		}
		else if (tFromLayout == vk::ImageLayout::ePreinitialized && tToLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
			imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		}
		else if (tFromLayout == vk::ImageLayout::eUndefined && tToLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
		{
			imageMemoryBarrier.srcAccessMask = {};
			imageMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		}
		else 
		{
			throw std::invalid_argument("Unsupported layout transition");
		}

		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = tImage->getHandle();
		imageMemoryBarrier.newLayout = tToLayout;
		imageMemoryBarrier.oldLayout = tFromLayout;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.subresourceRange.aspectMask = ImageBase::formatToAspectMask(tImage->getFormat());
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;
		imageMemoryBarrier.subresourceRange.levelCount = 1;

		// This class is a friend of the image class - store its new layout.
		tImage->mCurrentLayout = tToLayout;

		mCommandBufferHandle.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe, {}, {}, {}, imageMemoryBarrier);
	}

	void CommandBuffer::end()
	{
		mCommandBufferHandle.end();
	}

} // namespace graphics