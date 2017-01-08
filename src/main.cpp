#include <chrono>

#include "Vk.h"

float getElapsedSeconds()
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;
	return elapsed;
}

int main()
{
	/// vk::Instance
	auto instance = vk::Instance::create();
	auto physicalDevices = instance->getPhysicalDevices();
	assert(physicalDevices.size() > 0);

	/// vk::Window
	auto windowOptions = vk::Window::Options()
		.width(640)
		.height(480)
		.title("Test Application");
	auto window = vk::Window::create(instance, windowOptions);

	/// vk::Surface
	auto surface = window->createSurface();

	/// vk::Device
	auto deviceOptions = vk::Device::Options()
		.requiredQueueFlags(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
	auto device = vk::Device::create(physicalDevices[0], deviceOptions);

	auto queueFamilyProperties = device->getPhysicalDeviceQueueFamilyProperties();
	for (size_t i = 0; i < queueFamilyProperties.size(); ++i)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device->getPhysicalDeviceHandle(), static_cast<uint32_t>(i), surface->getHandle(), &presentSupport);

		if (presentSupport)
		{
			std::cout << "Queue Family at index " << i << " supports presentation\n";
		}
	}

	/// vk::Swapchain
	auto swapchain = vk::Swapchain::create(device, surface);

	/// vk::RenderPass
	auto renderPass = vk::RenderPass::create(device);

	/// vk::Pipeline
	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(float);
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::vector<VkPushConstantRange> pushConstantRanges = { pushConstantRange };
	auto pipelineOptions = vk::Pipeline::Options()
		.pushConstantRanges(pushConstantRanges);
	
	auto pipeline = vk::Pipeline::create(device, renderPass, pipelineOptions);

	/// vk::Framebuffer
	auto swapchainImageViews = swapchain->getSwapchainImageViews();
	std::vector<vk::FramebufferRef> framebuffers(swapchainImageViews.size());
	for (size_t i = 0; i < swapchainImageViews.size(); ++i)
	{
		std::vector<VkImageView> imageViews = { swapchainImageViews[i] };
		framebuffers[i] = vk::Framebuffer::create(device, renderPass, imageViews);
	}

	/// vk::CommandPool
	auto commandPoolOptions = vk::CommandPool::Options()
		.commandPoolCreateFlags(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	uint32_t queueFamilyIndex = device->getQueueFamilyIndices().mGraphicsIndex;
	auto commandPool = vk::CommandPool::create(queueFamilyIndex, device, commandPoolOptions);

	/// vk::CommandBuffer
	std::vector<vk::CommandBufferRef> commandBuffers(framebuffers.size());
	for (size_t i = 0; i < framebuffers.size(); ++i)
	{
		commandBuffers[i] = vk::CommandBuffer::create(device, commandPool);
		
		commandBuffers[i]->begin();
		commandBuffers[i]->beginRenderPass(renderPass, framebuffers[i]);
		commandBuffers[i]->bindPipeline(pipeline);
		commandBuffers[i]->draw(3, 1, 0, 0);
		commandBuffers[i]->endRenderPass();
		commandBuffers[i]->end();
	}
	
	/// vk::Semaphore
	auto imageAvailableSemaphore = vk::Semaphore::create(device);
	auto renderFinishedSemaphore = vk::Semaphore::create(device);

	// Main draw loop
	// https://developer.nvidia.com/vulkan-shader-resource-binding

	auto startTime = std::chrono::high_resolution_clock::now();

	while (!window->shouldWindowClose())
	{
		window->pollEvents();

		// Get the index of the next available image.
		uint32_t imageIndex = swapchain->acquireNextSwapchainImage(imageAvailableSemaphore);

		float elapsed = getElapsedSeconds();

		commandBuffers[imageIndex]->begin();
		commandBuffers[imageIndex]->beginRenderPass(renderPass, framebuffers[imageIndex]);
		commandBuffers[imageIndex]->bindPipeline(pipeline);
		commandBuffers[imageIndex]->updatePushConstantRanges(pipeline, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &elapsed);
		commandBuffers[imageIndex]->draw(3, 1, 0, 0);
		commandBuffers[imageIndex]->endRenderPass();
		commandBuffers[imageIndex]->end();

		// Re-record the entire command buffer due to push constants (?)
		auto commandBufferHandle = commandBuffers[imageIndex]->getHandle();

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphore->getHandle() };
		VkSemaphore signalSemaphores[] = { renderFinishedSemaphore->getHandle() };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;		// Wait for the image to be acquired from the swapchain.
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBufferHandle;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;	// Signal that rendering has finished.

		auto result = vkQueueSubmit(device->getQueueHandles().mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		assert(result == VK_SUCCESS);

		// Submit the result back to the swapchain for presentation:  make sure to
		// wait for rendering to finish before attempting to present.
		VkSwapchainKHR swapchains[] = { swapchain->getHandle() };
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;		
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		// This is equivalent to submitting a fence to a queue and waiting with an infinite timeout for that fence to signal.
		vkQueueWaitIdle(device->getQueueHandles().mGraphicsQueue);

		// Finally, present the image to the screen.
		vkQueuePresentKHR(device->getQueueHandles().mPresentationQueue, &presentInfo);
	}

	return 0;
}