#include <chrono>

#include "Vk.h"
#include "Geometry.h"

#include "glm/glm/gtc/matrix_transform.hpp"

struct UniformBufferData
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
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
	auto instance = vksp::Instance::create();
	auto physicalDevices = instance->getPhysicalDevices();
	assert(physicalDevices.size() > 0);

	/// vk::Window
	auto windowOptions = vksp::Window::Options().title("Vulkan Application");
	auto window = vksp::Window::create(instance, width, height, windowOptions);

	/// vk::Surface
	auto surface = window->createSurface();

	/// vk::Device
	auto deviceOptions = vksp::Device::Options().requiredQueueFlags(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer);
	auto device = vksp::Device::create(physicalDevices[0], deviceOptions);
	auto queueFamilyProperties = device->getPhysicalDeviceQueueFamilyProperties();
	for (size_t i = 0; i < queueFamilyProperties.size(); ++i)
	{
		vk::Bool32 presentSupport = device->getPhysicalDeviceHandle().getSurfaceSupportKHR(static_cast<uint32_t>(i), surface->getHandle());		
		if (presentSupport) { /* TODO: move this check into the device class */ }
	}
	std::cout << device << std::endl;

	/// vk::Swapchain
	auto swapchain = vksp::Swapchain::create(device, surface, width, height);
	auto swapchainImageViews = swapchain->getImageViewHandles();

	/// vk::RenderPass
	auto renderPass = vksp::RenderPass::create(device);

	/// vk::Buffer
	auto geometry = geo::Grid();
	
	auto vertexBuffer0 = vksp::Buffer::create(device, vk::BufferUsageFlagBits::eVertexBuffer, geometry.getPositions());
	auto vertexBuffer1 = vksp::Buffer::create(device, vk::BufferUsageFlagBits::eVertexBuffer, geometry.getTextureCoordinates());
	auto indexBuffer = vksp::Buffer::create(device, vk::BufferUsageFlagBits::eIndexBuffer, geometry.getIndices());
	auto uniformBuffer = vksp::Buffer::create(device, vk::BufferUsageFlagBits::eUniformBuffer, sizeof(UniformBufferData), nullptr);
	std::vector<vksp::BufferRef> vertexBuffers = { vertexBuffer0, vertexBuffer1 };

	/// vk::Pipeline
	auto bindingDescription0 = vksp::Pipeline::createVertexInputBindingDescription(0, sizeof(float) * 3);
	auto bindingDescription1 = vksp::Pipeline::createVertexInputBindingDescription(1, sizeof(float) * 2);
	auto attributeDescription0 = vksp::Pipeline::createVertexInputAttributeDescription(0, vk::Format::eR32G32B32Sfloat, 0, 0);	// 3 floats: position
	auto attributeDescription1 = vksp::Pipeline::createVertexInputAttributeDescription(1, vk::Format::eR32G32Sfloat, 1, 0);		// 2 floats: uv

	std::vector<vk::VertexInputBindingDescription> vertexInputBindingDescriptions = { bindingDescription0, bindingDescription1 };
	std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions = { attributeDescription0, attributeDescription1 };

	auto vertexShader = vksp::ShaderModule::create(device, "assets/shaders/vert.spv");
	auto fragmentShader = vksp::ShaderModule::create(device, "assets/shaders/frag.spv");

	auto pipelineOptions = vksp::Pipeline::Options()
		.vertexInputBindingDescriptions(vertexInputBindingDescriptions)
		.vertexInputAttributeDescriptions(vertexInputAttributeDescriptions)
		.viewport(window->getFullscreenViewport())
		.scissor(window->getFullscreenScissorRect2D())
		.attachShaderStage(vertexShader, vk::ShaderStageFlagBits::eVertex)
		.attachShaderStage(fragmentShader, vk::ShaderStageFlagBits::eFragment)
		.cullMode(vk::CullModeFlagBits::eNone)
		.primitiveTopology(geometry.getTopology());
	auto pipeline = vksp::Pipeline::create(device, renderPass, pipelineOptions);
	std::cout << pipeline << std::endl;

	/// vk::DescriptorPool
	vk::DescriptorPool descriptorPool = pipeline->createCompatibleDescriptorPool(0);
	
	/// vk::DescriptorSet
	vk::DescriptorSetLayout descriptorSetLayout = pipeline->getDescriptorSetLayout(0);
	vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(descriptorPool, 1, &descriptorSetLayout);
		
	vk::DescriptorSet descriptorSet = device->getHandle().allocateDescriptorSets(descriptorSetAllocateInfo)[0];

	vk::DescriptorBufferInfo descriptorBufferInfo{ uniformBuffer->getHandle(), 0, uniformBuffer->getRequestedSize() };
	vk::WriteDescriptorSet writeDescriptorSet{ descriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descriptorBufferInfo };

	vkUpdateDescriptorSets(device->getHandle(), 1, &static_cast<VkWriteDescriptorSet>(writeDescriptorSet), 0, nullptr);

	/// vk::Framebuffer
	std::vector<vksp::FramebufferRef> framebuffers(swapchainImageViews.size());
	for (size_t i = 0; i < swapchainImageViews.size(); ++i)
	{
		std::vector<vk::ImageView> imageViews = { swapchainImageViews[i] };
		framebuffers[i] = vksp::Framebuffer::create(device, renderPass, imageViews, width, height);
	}

	/// vk::CommandPool
	auto commandPool = vksp::CommandPool::create(device, device->getQueueFamiliesMapping().graphics().second);

	/// vk::CommandBuffer
	std::vector<vksp::CommandBufferRef> commandBuffers(framebuffers.size(), vksp::CommandBuffer::create(device, commandPool));
	
	/// vk::Semaphore
	auto imageAvailableSemaphore = vksp::Semaphore::create(device);
	auto renderFinishedSemaphore = vksp::Semaphore::create(device);

	UniformBufferData ubo = {};
	ubo.model = glm::mat4();
	ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.projection = glm::perspective(glm::radians(45.0f), width / (float)height, 0.1f, 1000.0f);

	auto startTime = std::chrono::high_resolution_clock::now();
	while (!window->shouldWindowClose())
	{
		window->pollEvents();

		// Re-record the entire command buffer due to push constants (?)
		float elapsed = getElapsedSeconds();
		glm::vec2 mousePosition = window->getMousePosition();
		mousePosition.x /= width;
		mousePosition.y /= height;

		ubo.model = glm::rotate(glm::mat4(), elapsed * 0.25f, glm::vec3(0.0, 0.0, 1.0f));

		// Update the uniform buffer object.
		void *data = uniformBuffer->getDeviceMemory()->map(0, uniformBuffer->getDeviceMemory()->getAllocationSize());
		memcpy(data, &ubo, sizeof(ubo));
		uniformBuffer->getDeviceMemory()->unmap();

		// Get the index of the next available image.
		uint32_t imageIndex = swapchain->acquireNextSwapchainImage(imageAvailableSemaphore);
		commandBuffers[imageIndex]->begin();
		commandBuffers[imageIndex]->beginRenderPass(renderPass, framebuffers[imageIndex]);
		commandBuffers[imageIndex]->bindPipeline(pipeline);
		commandBuffers[imageIndex]->bindVertexBuffers(vertexBuffers);
		commandBuffers[imageIndex]->bindIndexBuffer(indexBuffer);
		commandBuffers[imageIndex]->updatePushConstantRanges(pipeline, "time", &elapsed);
		commandBuffers[imageIndex]->updatePushConstantRanges(pipeline, "mouse", &mousePosition);
		commandBuffers[imageIndex]->getHandle().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline->getPipelineLayoutHandle(), 0, 1, &descriptorSet, 0, nullptr);
		commandBuffers[imageIndex]->drawIndexed(static_cast<uint32_t>(geometry.getIndices().size()), 1, 0, 0, 0);
		commandBuffers[imageIndex]->endRenderPass();
		commandBuffers[imageIndex]->end();
		
		auto commandBufferHandle = commandBuffers[imageIndex]->getHandle();
		vk::Semaphore waitSemaphores[] = { imageAvailableSemaphore->getHandle() };
		vk::Semaphore signalSemaphores[] = { renderFinishedSemaphore->getHandle() };
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		
		vk::SubmitInfo submitInfo;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;		// Wait for the image to be acquired from the swapchain.
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBufferHandle;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;	// Signal that rendering has finished.

		device->getQueueFamiliesMapping().graphics().first.submit(submitInfo, VK_NULL_HANDLE);

		// Submit the result back to the swapchain for presentation:  make sure to wait for rendering to finish before attempting to present.
		vk::SwapchainKHR swapchains[] = { swapchain->getHandle() };

		vk::PresentInfoKHR presentInfo;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;		
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		// This is equivalent to submitting a fence to a queue and waiting with an infinite timeout for that fence to signal.
		device->getQueueFamiliesMapping().graphics().first.waitIdle();
		device->getQueueFamiliesMapping().presentation().first.presentKHR(presentInfo);
	}

	return 0;
}
