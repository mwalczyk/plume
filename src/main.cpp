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
	const uint32_t width = 800;
	const uint32_t height = 600;

	/// vk::Instance
	auto instance = vk::Instance::create();
	auto physicalDevices = instance->getPhysicalDevices();
	assert(physicalDevices.size() > 0);

	/// vk::Window
	auto windowOptions = vk::Window::Options()
		.width(width)
		.height(height)
		.title("Vulkan Application");
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
	auto swapchainOptions = vk::Swapchain::Options()
		.width(width)
		.height(height);
	auto swapchain = vk::Swapchain::create(device, surface, swapchainOptions);

	/// vk::RenderPass
	auto renderPass = vk::RenderPass::create(device);

	/// vk::Buffer
	static const std::vector<float> vertices = {
		0.0f, -0.5f, 1.0f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
		-0.5f, 0.5f, 0.0f, 0.0f, 1.0f
	};
	auto vertexBuffer = vk::Buffer::create(device, vertices);
	std::vector<vk::BufferRef> vertexBuffers = { vertexBuffer };

	/// vk::Pipeline
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	bindingDescription.stride = sizeof(float) * 5;
	
	VkVertexInputAttributeDescription attributeDescriptionPosition = {};
	attributeDescriptionPosition.binding = 0;
	attributeDescriptionPosition.format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptionPosition.location = 0;
	attributeDescriptionPosition.offset = 0;

	VkVertexInputAttributeDescription attributeDescriptionColor = {};
	attributeDescriptionColor.binding = 0;
	attributeDescriptionColor.format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptionColor.location = 1;
	attributeDescriptionColor.offset = sizeof(float) * 2;

	std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions = { bindingDescription };
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions = { attributeDescriptionPosition, attributeDescriptionColor };

	auto vertexShader = vk::ShaderModule::create(device, "assets/shaders/vert.spv");
	auto fragmentShader = vk::ShaderModule::create(device, "assets/shaders/frag.spv");
	auto pipelineOptions = vk::Pipeline::Options()
		.vertexInputBindingDescriptions(vertexInputBindingDescriptions)
		.vertexInputAttributeDescriptions(vertexInputAttributeDescriptions)
		.viewport(window->getFullscreenViewport())
		.scissor(window->getFullscreenScissorRect2D())
		.vertexShader(vertexShader)
		.fragmentShader(fragmentShader);
	auto pipeline = vk::Pipeline::create(device, renderPass, pipelineOptions);

	/// vk::Framebuffer
	auto swapchainImageViews = swapchain->getSwapchainImageViews();
	std::vector<vk::FramebufferRef> framebuffers(swapchainImageViews.size());
	for (size_t i = 0; i < swapchainImageViews.size(); ++i)
	{
		std::vector<VkImageView> imageViews = { swapchainImageViews[i] };
		auto frameBufferOptions = vk::Framebuffer::Options()
			.width(width)
			.height(height);
		framebuffers[i] = vk::Framebuffer::create(device, renderPass, imageViews, frameBufferOptions);
	}

	/// vk::CommandPool
	auto commandPoolOptions = vk::CommandPool::Options()
		.commandPoolCreateFlags(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	auto commandPool = vk::CommandPool::create(device, device->getQueueFamilyIndices().mGraphicsIndex, commandPoolOptions);

	/// vk::CommandBuffer
	std::vector<vk::CommandBufferRef> commandBuffers(framebuffers.size(), vk::CommandBuffer::create(device, commandPool));
	
	/// vk::Semaphore
	auto imageAvailableSemaphore = vk::Semaphore::create(device);
	auto renderFinishedSemaphore = vk::Semaphore::create(device);
	auto startTime = std::chrono::high_resolution_clock::now();

	while (!window->shouldWindowClose())
	{
		window->pollEvents();

		// Get the index of the next available image.
		uint32_t imageIndex = swapchain->acquireNextSwapchainImage(imageAvailableSemaphore);

		// Re-record the entire command buffer due to push constants (?)
		float elapsed = getElapsedSeconds();
		glm::vec2 mousePosition = window->getMousePosition();
		mousePosition.x /= width;
		mousePosition.y /= height;

		commandBuffers[imageIndex]->begin();
		commandBuffers[imageIndex]->beginRenderPass(renderPass, framebuffers[imageIndex]);
		commandBuffers[imageIndex]->bindPipeline(pipeline);
		commandBuffers[imageIndex]->bindVertexBuffers(vertexBuffers);
		commandBuffers[imageIndex]->updatePushConstantRanges(pipeline, "time", &elapsed);
		commandBuffers[imageIndex]->updatePushConstantRanges(pipeline, "mouse", &mousePosition);
		commandBuffers[imageIndex]->draw(3, 1, 0, 0);
		commandBuffers[imageIndex]->endRenderPass();
		commandBuffers[imageIndex]->end();
		
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

		// Submit the result back to the swapchain for presentation:  make sure to wait for rendering to finish before attempting to present.
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
