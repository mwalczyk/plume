#include <chrono>

#include "Vk.h"

#include "glm/glm/gtc/matrix_transform.hpp"

static const std::vector<float> vertices = {
   -1.0f, -1.0f,	1.0f, 0.0f, 0.0f,	0.0f, 1.0f,
    1.0f, -1.0f,	0.0f, 1.0f, 0.0f,	1.0f, 1.0f,
    1.0f, 1.0f,	    0.0f, 0.0f, 1.0f,	1.0f, 0.0f,
   -1.0f, 1.0f,		1.0f, 1.0f, 1.0f,	0.0f, 0.0f
};

static const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
};

struct UniformBufferData
{
	glm::mat4 model;
};

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
	const uint32_t height = 800;

	/// vk::Instance
	auto instance = vk::Instance::create();
	auto physicalDevices = instance->getPhysicalDevices();
	assert(physicalDevices.size() > 0);

	/// vk::Window
	auto windowOptions = vk::Window::Options().title("Vulkan Application");
	auto window = vk::Window::create(instance, width, height, windowOptions);

	/// vk::Surface
	auto surface = window->createSurface();

	/// vk::Device
	auto deviceOptions = vk::Device::Options().requiredQueueFlags(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
	auto device = vk::Device::create(physicalDevices[0], deviceOptions);
	auto queueFamilyProperties = device->getPhysicalDeviceQueueFamilyProperties();
	for (size_t i = 0; i < queueFamilyProperties.size(); ++i)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device->getPhysicalDeviceHandle(), static_cast<uint32_t>(i), surface->getHandle(), &presentSupport);

		if (presentSupport)
		{
			// TODO: move this check into the device class
		}
	}
	std::cout << device << std::endl;

	/// vk::Swapchain
	auto swapchain = vk::Swapchain::create(device, surface, width, height);
	auto swapchainImageViews = swapchain->getImageViewHandles();

	/// vk::RenderPass
	auto renderPass = vk::RenderPass::create(device);

	/// vk::Buffer
	auto vertexBuffer = vk::Buffer::create(device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices);
	auto indexBuffer = vk::Buffer::create(device, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices);
	auto uniformBuffer = vk::Buffer::create(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(UniformBufferData), nullptr);
	std::vector<vk::BufferRef> vertexBuffers = { vertexBuffer };

	/// vk::Pipeline
	auto bindingDescription = vk::Pipeline::createVertexInputBindingDescription(0, sizeof(float) * 7);
	auto attributeDescriptionPosition = vk::Pipeline::createVertexInputAttributeDescription(0, VK_FORMAT_R32G32_SFLOAT, 0, 0);
	auto attributeDescriptionColor =	vk::Pipeline::createVertexInputAttributeDescription(0, VK_FORMAT_R32G32B32_SFLOAT, 1, sizeof(float) * 2);
	auto attributeDescriptionTexcoord = vk::Pipeline::createVertexInputAttributeDescription(0, VK_FORMAT_R32G32_SFLOAT, 2, sizeof(float) * 5);

	std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions = { bindingDescription };
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions = { attributeDescriptionPosition, attributeDescriptionColor, attributeDescriptionTexcoord };

	auto vertexShader = vk::ShaderModule::create(device, "../assets/shaders/vert.spv");
	auto fragmentShader = vk::ShaderModule::create(device, "../assets/shaders/frag.spv");
	auto pipelineOptions = vk::Pipeline::Options()
		.vertexInputBindingDescriptions(vertexInputBindingDescriptions)
		.vertexInputAttributeDescriptions(vertexInputAttributeDescriptions)
		.viewport(window->getFullscreenViewport())
		.scissor(window->getFullscreenScissorRect2D())
		.attachShaderStage(vertexShader, VK_SHADER_STAGE_VERTEX_BIT)
		.attachShaderStage(fragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT);
	auto pipeline = vk::Pipeline::create(device, renderPass, pipelineOptions);
	std::cout << pipeline << std::endl;

	/// vk::DescriptorPool
	VkDescriptorPool descriptorPool = pipeline->createCompatibleDescriptorPool(0);
	
	/// vk::DescriptorSet
	VkDescriptorSetLayout descriptorSetLayout = pipeline->getDescriptorSetLayout(0);
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;
		
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	auto result = vkAllocateDescriptorSets(device->getHandle(), &descriptorSetAllocateInfo, &descriptorSet);
	assert(result == VK_SUCCESS);

	VkDescriptorBufferInfo descriptorBufferInfo = {};
	descriptorBufferInfo.buffer = uniformBuffer->getHandle();
	descriptorBufferInfo.offset = 0;
	descriptorBufferInfo.range = uniformBuffer->getSize();

	VkWriteDescriptorSet writeDescriptorSet = {};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = descriptorSet;
	writeDescriptorSet.dstBinding = 0;
	writeDescriptorSet.dstArrayElement = 0;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

	vkUpdateDescriptorSets(device->getHandle(), 1, &writeDescriptorSet, 0, nullptr);

	/// vk::Framebuffer
	std::vector<vk::FramebufferRef> framebuffers(swapchainImageViews.size());
	for (size_t i = 0; i < swapchainImageViews.size(); ++i)
	{
		std::vector<VkImageView> imageViews = { swapchainImageViews[i] };
		framebuffers[i] = vk::Framebuffer::create(device, renderPass, imageViews, width, height);
	}

	/// vk::CommandPool
	auto commandPoolOptions = vk::CommandPool::Options().commandPoolCreateFlags(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
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

		// Re-record the entire command buffer due to push constants (?)
		float elapsed = getElapsedSeconds();
		glm::vec2 mousePosition = window->getMousePosition();
		mousePosition.x /= width;
		mousePosition.y /= height;

		// Update the uniform buffer object.
		UniformBufferData ubo = {};
		ubo.model = glm::mat4(); //glm::rotate(glm::mat4(), elapsed * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		void *data = uniformBuffer->map(0, uniformBuffer->getSize());
		memcpy(data, &ubo, sizeof(ubo));
		uniformBuffer->unmap();

		// Get the index of the next available image.
		uint32_t imageIndex = swapchain->acquireNextSwapchainImage(imageAvailableSemaphore);

		commandBuffers[imageIndex]->begin();
		commandBuffers[imageIndex]->beginRenderPass(renderPass, framebuffers[imageIndex]);
		commandBuffers[imageIndex]->bindPipeline(pipeline);
		commandBuffers[imageIndex]->bindVertexBuffers(vertexBuffers);
		commandBuffers[imageIndex]->bindIndexBuffer(indexBuffer);
		commandBuffers[imageIndex]->updatePushConstantRanges(pipeline, "time", &elapsed);
		commandBuffers[imageIndex]->updatePushConstantRanges(pipeline, "mouse", &mousePosition);
		vkCmdBindDescriptorSets(commandBuffers[imageIndex]->getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getPipelineLayoutHandle(), 0, 1, &descriptorSet, 0, nullptr);
		commandBuffers[imageIndex]->drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
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
