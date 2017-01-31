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
	auto instance = graphics::Instance::create();
	auto physicalDevices = instance->getPhysicalDevices();

	/// vk::Window
	auto window = graphics::Window::create(instance, width, height);

	/// vk::Surface
	auto surface = window->create_surface();

	/// vk::Device
	auto deviceOptions = graphics::Device::Options().requiredQueueFlags(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer);
	auto device = graphics::Device::create(physicalDevices[0], deviceOptions);
	auto queueFamilyProperties = device->getPhysicalDeviceQueueFamilyProperties();
	for (size_t i = 0; i < queueFamilyProperties.size(); ++i)
	{
		vk::Bool32 presentSupport = device->getPhysicalDeviceHandle().getSurfaceSupportKHR(static_cast<uint32_t>(i), surface->get_handle());		
		if (presentSupport) { /* TODO: move this check into the device class */ }
	}
	std::cout << device << std::endl;

	/// vk::Swapchain
	auto swapchain = graphics::Swapchain::create(device, surface, width, height);
	auto swapchainImageViews = swapchain->get_image_view_handles();

	/// vk::RenderPass
	auto renderPass = graphics::RenderPass::create(device);

	/// vk::Buffer
	auto geometry = geo::IcoSphere();
	geometry.setRandomColors();
	
	auto vertexBuffer0 = graphics::Buffer::create(device, vk::BufferUsageFlagBits::eVertexBuffer, geometry.getPositions());
	auto vertexBuffer1 = graphics::Buffer::create(device, vk::BufferUsageFlagBits::eVertexBuffer, geometry.getColors());
	auto indexBuffer = graphics::Buffer::create(device, vk::BufferUsageFlagBits::eIndexBuffer, geometry.getIndices());
	auto uniformBuffer = graphics::Buffer::create(device, vk::BufferUsageFlagBits::eUniformBuffer, sizeof(UniformBufferData), nullptr);
	std::vector<graphics::BufferRef> vertexBuffers = { vertexBuffer0, vertexBuffer1 };

	/// vk::Pipeline
	auto bindingDescription0 = graphics::Pipeline::createVertexInputBindingDescription(0, sizeof(float) * 3);
	auto bindingDescription1 = graphics::Pipeline::createVertexInputBindingDescription(1, sizeof(float) * 3);
	auto attributeDescription0 = graphics::Pipeline::createVertexInputAttributeDescription(0, vk::Format::eR32G32B32Sfloat, 0, 0);		// 3 floats: position
	auto attributeDescription1 = graphics::Pipeline::createVertexInputAttributeDescription(1, vk::Format::eR32G32B32Sfloat, 1, 0);		// 3 floats: color

	std::vector<vk::VertexInputBindingDescription> vertexInputBindingDescriptions = { bindingDescription0, bindingDescription1 };
	std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions = { attributeDescription0, attributeDescription1 };
	
	auto vertexShader = graphics::ShaderModule::create(device, ResourceManager::loadFile("assets/shaders/vert.spv"));
	auto fragmentShader = graphics::ShaderModule::create(device, ResourceManager::loadFile("assets/shaders/frag.spv"));

	auto pipelineOptions = graphics::Pipeline::Options()
		.vertexInputBindingDescriptions(vertexInputBindingDescriptions)
		.vertexInputAttributeDescriptions(vertexInputAttributeDescriptions)
		.viewport(window->get_fullscreen_viewport())
		.scissor(window->get_fullscreen_scissor_rect2d())
		.attachShaderStage(vertexShader, vk::ShaderStageFlagBits::eVertex)
		.attachShaderStage(fragmentShader, vk::ShaderStageFlagBits::eFragment)
		.cullMode(vk::CullModeFlagBits::eNone)
		.primitiveTopology(geometry.getTopology())
		.depthTest();
	auto pipeline = graphics::Pipeline::create(device, renderPass, pipelineOptions);
	std::cout << pipeline << std::endl;

	/// vk::CommandPool
	auto commandPool = graphics::CommandPool::create(device, device->getQueueFamiliesMapping().graphics().second);

	/// vk::CommandBuffer
	std::vector<graphics::CommandBufferRef> commandBuffers(swapchainImageViews.size(), graphics::CommandBuffer::create(device, commandPool));
	
	/// vk::Image
	auto texture = graphics::Image2D::create(device, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::Format::eR8G8B8A8Unorm, ResourceManager::loadImage("assets/textures/texture.jpg"));
	auto textureView = texture->build_image_view();
	auto textureSampler = texture->build_sampler();
	
	auto depthImageOptions = graphics::Image2D::Options().image_tiling(vk::ImageTiling::eOptimal);
	auto depthImage = graphics::Image2D::create(device, vk::ImageUsageFlagBits::eDepthStencilAttachment, device->getSupportedDepthFormat(), width, height, depthImageOptions);
	auto depthImageView = depthImage->build_image_view();

	{
		auto temporaryCommandBuffer = graphics::CommandBuffer::create(device, commandPool);
		auto temporaryCommandBufferHandle = temporaryCommandBuffer->get_handle();
		temporaryCommandBuffer->begin();
		temporaryCommandBuffer->transition_image_layout(texture, vk::ImageLayout::ePreinitialized, vk::ImageLayout::eShaderReadOnlyOptimal);
		temporaryCommandBuffer->transition_image_layout(depthImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
		temporaryCommandBuffer->end();

		vk::SubmitInfo submitInfo;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &temporaryCommandBufferHandle;
		device->getQueueFamiliesMapping().graphics().first.submit(submitInfo, {});
		device->getQueueFamiliesMapping().graphics().first.waitIdle();
	}

	/// vk::Framebuffer
	std::vector<graphics::FramebufferRef> framebuffers(swapchainImageViews.size());
	for (size_t i = 0; i < swapchainImageViews.size(); ++i)
	{
		std::vector<vk::ImageView> imageViews = { swapchainImageViews[i], depthImageView };
		framebuffers[i] = graphics::Framebuffer::create(device, renderPass, imageViews, width, height);
	}

	/// vk::DescriptorPool
	auto descriptorPool = graphics::DescriptorPool::create(device, { {vk::DescriptorType::eUniformBuffer, 1}, {vk::DescriptorType::eCombinedImageSampler, 1} } );

	/// vk::DescriptorSet
	vk::DescriptorSetLayout descriptorSetLayout = pipeline->getDescriptorSetLayout(0);
	vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(descriptorPool->getHandle(), 1, &descriptorSetLayout);

	vk::DescriptorSet descriptorSet = device->getHandle().allocateDescriptorSets(descriptorSetAllocateInfo)[0];

	auto descriptorBufferInfo = uniformBuffer->build_descriptor_info();							// ubo
	auto descriptorImageInfo = texture->build_descriptor_info(textureSampler, textureView);		// sampler

	vk::WriteDescriptorSet writeDescriptorSetBuffer = { descriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descriptorBufferInfo };	// ubo
	vk::WriteDescriptorSet writeDescriptorSetSampler = { descriptorSet, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &descriptorImageInfo };		// sampler
	std::vector<vk::WriteDescriptorSet> writeDescriptorSets = { writeDescriptorSetBuffer, writeDescriptorSetSampler };

	device->getHandle().updateDescriptorSets(writeDescriptorSets, {});

	/// vk::Semaphore
	auto imageAvailableSemaphore = graphics::Semaphore::create(device);
	auto renderFinishedSemaphore = graphics::Semaphore::create(device);

	UniformBufferData ubo = {};
	ubo.model = glm::translate(glm::mat4(), glm::vec3(0.0, 0.0, -2.0f));
	ubo.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	ubo.projection = glm::perspective(glm::radians(45.0f), width / (float)height, 0.1f, 1000.0f);

	// Update the uniform buffer object.
	void *data = uniformBuffer->get_device_memory()->map(0, uniformBuffer->get_device_memory()->get_allocation_size());
	memcpy(data, &ubo, sizeof(ubo));
	uniformBuffer->get_device_memory()->unmap();

	while (!window->should_close())
	{
		window->poll_events();

		// Re-record the entire command buffer due to push constants (?)
		float elapsed = getElapsedSeconds();
		glm::vec2 mousePosition = window->get_mouse_position();
		mousePosition.x /= width;
		mousePosition.y /= height;

		// Get the index of the next available image.
		uint32_t imageIndex = swapchain->acquire_next_swapchain_image(imageAvailableSemaphore);

		// Set the clear values for each of this framebuffer's attachments, including the depth stencil attachment.
		std::vector<vk::ClearValue> clearValues(2);
		clearValues[0].color = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[1].depthStencil = {1, 0};
		
		commandBuffers[imageIndex]->begin();
		commandBuffers[imageIndex]->begin_render_pass(renderPass, framebuffers[imageIndex], clearValues);
		commandBuffers[imageIndex]->bind_pipeline(pipeline);
		commandBuffers[imageIndex]->bind_vertex_buffers(vertexBuffers);
		commandBuffers[imageIndex]->bind_index_buffer(indexBuffer);
		commandBuffers[imageIndex]->update_push_constant_ranges(pipeline, "time", &elapsed);
		commandBuffers[imageIndex]->update_push_constant_ranges(pipeline, "mouse", &mousePosition);
		commandBuffers[imageIndex]->get_handle().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline->getPipelineLayoutHandle(), 0, 1, &descriptorSet, 0, nullptr);
		commandBuffers[imageIndex]->draw_indexed(static_cast<uint32_t>(geometry.getIndices().size()), 1, 0, 0, 0);
		commandBuffers[imageIndex]->end_render_pass();
		commandBuffers[imageIndex]->end();
		
		auto commandBufferHandle = commandBuffers[imageIndex]->get_handle();
		vk::Semaphore waitSemaphores[] = { imageAvailableSemaphore->get_handle() };
		vk::Semaphore signalSemaphores[] = { renderFinishedSemaphore->get_handle() };
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		
		vk::SubmitInfo submitInfo;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;		// Wait for the image to be acquired from the swapchain.
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBufferHandle;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;	// Signal that rendering has finished.

		device->getQueueFamiliesMapping().graphics().first.submit(submitInfo, {});

		// Submit the result back to the swapchain for presentation:  make sure to wait for rendering to finish before attempting to present.
		vk::SwapchainKHR swapchains[] = { swapchain->get_handle() };

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
