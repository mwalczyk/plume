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

static const uint32_t width = 800;
static const uint32_t height = 800;

int main()
{
	/// vk::Instance
	auto instance = graphics::Instance::create();
	auto physical_devices = instance->get_physical_devices();

	/// vk::Window
	auto window = graphics::Window::create(instance, width, height);

	/// vk::Surface
	auto surface = window->create_surface();

	/// vk::Device
	auto device_options = graphics::Device::Options().required_queue_flags(vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer);
	auto device = graphics::Device::create(physical_devices[0], device_options);
	auto queue_family_properties = device->get_physical_device_queue_family_properties();
	for (size_t i = 0; i < queue_family_properties.size(); ++i)
	{
		vk::Bool32 support = device->get_physical_device_handle().getSurfaceSupportKHR(static_cast<uint32_t>(i), surface->get_handle());
		if (support) { /* TODO: move this check into the device class */ }
	}
	std::cout << device << std::endl;

	/// vk::Swapchain
	auto swapchain = graphics::Swapchain::create(device, surface, width, height);
	auto swapchain_image_views = swapchain->get_image_view_handles();

	/// vk::RenderPass
	auto render_pass = graphics::RenderPass::create(device);

	/// geo::Geometry
	auto geometry = geo::Grid();
	geometry.set_solid({ 1.0f, 1.0f, 1.0f });

	/// vk::Buffer
	auto vbo_0 = graphics::Buffer::create(device, vk::BufferUsageFlagBits::eVertexBuffer, geometry.get_positions());
	auto vbo_1 = graphics::Buffer::create(device, vk::BufferUsageFlagBits::eVertexBuffer, geometry.get_colors());
	auto ibo = graphics::Buffer::create(device, vk::BufferUsageFlagBits::eIndexBuffer, geometry.get_indices());
	auto ubo = graphics::Buffer::create(device, vk::BufferUsageFlagBits::eUniformBuffer, sizeof(UniformBufferData), nullptr);

	/// vk::Pipeline
	vk::VertexInputBindingDescription binding_0 = { 0, sizeof(float) * 3 }; // input rate vertex: 3 floats between each vertex
	vk::VertexInputBindingDescription binding_1 = { 1, sizeof(float) * 3 }; // input rate vertex: 3 floats between each vertex
	vk::VertexInputAttributeDescription attr_0 = { 0, binding_0.binding, vk::Format::eR32G32B32Sfloat }; // 3 floats: position
	vk::VertexInputAttributeDescription attr_1 = { 1, binding_1.binding, vk::Format::eR32G32B32Sfloat }; // 3 floats: color
	auto v_shader = graphics::ShaderModule::create(device, ResourceManager::load_file("../assets/shaders/vert.spv"));
	auto f_shader = graphics::ShaderModule::create(device, ResourceManager::load_file("../assets/shaders/frag.spv"));

	auto pipeline_options = graphics::Pipeline::Options()
		.vertex_input_binding_descriptions({ binding_0, binding_1 })
		.vertex_input_attribute_descriptions({ attr_0, attr_1 })
		.viewports({ window->get_fullscreen_viewport() })
		.scissors({ window->get_fullscreen_scissor_rect2d() })
		.attach_shader_stage(v_shader, vk::ShaderStageFlagBits::eVertex)
		.attach_shader_stage(f_shader, vk::ShaderStageFlagBits::eFragment)
		.primitive_topology(geometry.get_topology());
	auto pipeline = graphics::Pipeline::create(device, render_pass, pipeline_options);
	std::cout << pipeline << std::endl;

	/// vk::CommandPool
	auto command_pool = graphics::CommandPool::create(device, device->get_queue_families_mapping().graphics().second, vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

	/// vk::Image
	auto texture = graphics::Image2D::create(device, vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::Format::eR8G8B8A8Unorm, ResourceManager::load_image("../assets/textures/texture.jpg"));
	auto texture_view = texture->build_image_view();
	auto texture_sampler = texture->build_sampler();

	auto depth_image_options = graphics::Image2D::Options().image_tiling(vk::ImageTiling::eOptimal);
	auto depth_image = graphics::Image2D::create(device, vk::ImageUsageFlagBits::eDepthStencilAttachment, device->get_supported_depth_format(), width, height, depth_image_options);
	auto depth_image_view = depth_image->build_image_view();

	{
		auto temp_command_buffer = graphics::CommandBuffer::create(device, command_pool);
		auto temp_command_buffer_handle = temp_command_buffer->get_handle();
		temp_command_buffer->begin();
		temp_command_buffer->transition_image_layout(texture, vk::ImageLayout::ePreinitialized, vk::ImageLayout::eShaderReadOnlyOptimal);
		temp_command_buffer->transition_image_layout(depth_image, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
		temp_command_buffer->end();

		vk::SubmitInfo submit_info;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &temp_command_buffer_handle;
		device->get_queue_families_mapping().graphics().first.submit(submit_info, {});
		device->get_queue_families_mapping().graphics().first.waitIdle();
	}

	/// vk::Framebuffer
	std::vector<graphics::FramebufferRef> framebuffers(swapchain_image_views.size());
	for (size_t i = 0; i < swapchain_image_views.size(); ++i)
	{
		std::vector<vk::ImageView> image_views = { swapchain_image_views[i], depth_image_view };
		framebuffers[i] = graphics::Framebuffer::create(device, render_pass, image_views, width, height);
	}

	/// vk::DescriptorPool
	auto descriptor_pool = graphics::DescriptorPool::create(device, { {vk::DescriptorType::eUniformBuffer, 1}, {vk::DescriptorType::eCombinedImageSampler, 1} } );

	/// vk::DescriptorSet
	vk::DescriptorSetLayout descriptor_set_layout = pipeline->get_descriptor_set_layout(0);
	vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(descriptor_pool->get_handle(), 1, &descriptor_set_layout);
	vk::DescriptorSet descriptor_set = device->get_handle().allocateDescriptorSets(descriptorSetAllocateInfo)[0];

	auto d_buffer_info = ubo->build_descriptor_info();										// ubo
	auto d_image_info = texture->build_descriptor_info(texture_sampler, texture_view);		// sampler
	vk::WriteDescriptorSet w_descriptor_set_buffer = { descriptor_set, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &d_buffer_info };		// ubo
	vk::WriteDescriptorSet w_descriptor_set_sampler = { descriptor_set, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &d_image_info };		// sampler
	std::vector<vk::WriteDescriptorSet> w_descriptor_sets = { w_descriptor_set_buffer, w_descriptor_set_sampler };
	device->get_handle().updateDescriptorSets(w_descriptor_sets, {});

	/// vk::Semaphore
	auto image_available_sem = graphics::Semaphore::create(device);
	auto render_complete_sem = graphics::Semaphore::create(device);

	UniformBufferData ubo_data = {};
	ubo_data.model = glm::translate(glm::mat4(), { 50.0f, 30.0, 0.0f });
	ubo_data.model = glm::scale(ubo_data.model, { 50.0f, 30.0f, 0.0f });
	ubo_data.view = glm::lookAt({ 0.0f, 0.0, 2.0f }, { 0.0f, 0.0, 0.0f }, glm::vec3(0.0f, 1.0f, 0.0f));
	ubo_data.projection = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);

	void *data = ubo->get_device_memory()->map(0, ubo->get_device_memory()->get_allocation_size());
	memcpy(data, &ubo_data, sizeof(ubo_data));
	ubo->get_device_memory()->unmap();

	while (!window->should_close())
	{
		window->poll_events();

		// Set up data for push constants.
		float elapsed = getElapsedSeconds();
		glm::vec2 mouse_position = window->get_mouse_position();
		mouse_position.x /= width;
		mouse_position.y /= height;
		glm::vec3 color = { 1.0f, mouse_position.x, 0.0f };

		// Get the index of the next available image.
		uint32_t image_index = swapchain->acquire_next_swapchain_image(image_available_sem);

		// Set the clear values for each of this framebuffer's attachments, including the depth stencil attachment.
		std::vector<vk::ClearValue> clear_vals(2);
		clear_vals[0].color = std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f };
		clear_vals[1].depthStencil = {1.0f, 0};

		// Set up a new command buffer and record draw calls.
		auto command_buffer = graphics::CommandBuffer::create(device, command_pool);
		auto command_buffer_handle = command_buffer->get_handle();
		command_buffer->begin();
		command_buffer->begin_render_pass(render_pass, framebuffers[image_index], clear_vals);
		command_buffer->bind_pipeline(pipeline);
		command_buffer->bind_vertex_buffers({ vbo_0, vbo_1 });
		command_buffer->bind_index_buffer(ibo);
		command_buffer->update_push_constant_ranges(pipeline, "time", &elapsed);
		command_buffer->update_push_constant_ranges(pipeline, "mouse", &mouse_position);
		command_buffer->update_push_constant_ranges(pipeline, "color", glm::value_ptr(color));
		command_buffer->get_handle().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline->get_pipeline_layout_handle(), 0, 1, &descriptor_set, 0, nullptr);
		command_buffer->draw_indexed(static_cast<uint32_t>(geometry.get_indices().size()), 1, 0, 0, 0);
		command_buffer->end_render_pass();
		command_buffer->end();

		// Prepare semaphores.
		vk::Semaphore wait_sems[] = { image_available_sem->get_handle() };
		vk::Semaphore signal_sems[] = { render_complete_sem->get_handle() };
		vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		vk::SubmitInfo submit_info;
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = wait_sems;
		submit_info.pWaitDstStageMask = wait_stages;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer_handle;
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = signal_sems;
		device->get_queue_families_mapping().graphics().first.submit(submit_info, {});

		// Submit the result back to the swapchain for presentation:  make sure to wait for rendering to finish before attempting to present.
		vk::SwapchainKHR swapchains[] = { swapchain->get_handle() };
		vk::PresentInfoKHR present_info;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signal_sems;
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swapchains;
		present_info.pImageIndices = &image_index;
		present_info.pResults = nullptr;

		// This is equivalent to submitting a fence to a queue and waiting with an infinite timeout for that fence to signal.
		device->get_queue_families_mapping().graphics().first.waitIdle();
		device->get_queue_families_mapping().presentation().first.presentKHR(present_info);
	}

	return 0;
}
