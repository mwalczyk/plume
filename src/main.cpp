#include "Vk.h"
#include "Geometry.h"

#include "glm/glm/gtc/matrix_transform.hpp"

struct UniformBufferData
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

// TODO: putting this in `utils.h` causes errors related to "multiply defined symbols"
//! Retrieve the number of seconds that have elapsed since the application started.
float get_elapsed_seconds()
{
	static auto start = std::chrono::high_resolution_clock::now();
	auto current = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current - start).count() / 1000.0f;

	return elapsed;
}

using namespace graphics;

static const uint32_t width = 800;
static const uint32_t height = 800;
static const uint32_t msaa = 8;

int main()
{
   /***********************************************************************************
	*
	* Instance, window, surface, device, and swapchain
	*
	***********************************************************************************/
	auto instance = Instance::create();
	auto physical_devices = instance->get_physical_devices();

	auto window = Window::create(instance, width, height);
	auto surface = window->create_surface();

	auto device = Device::create(physical_devices[0], surface);
	auto queue_family_properties = device->get_physical_device_queue_family_properties();
	std::cout << device << std::endl;

	auto swapchain = Swapchain::create(device, surface, width, height);
	auto swapchain_image_views = swapchain->get_image_view_handles();

   /***********************************************************************************
	*
	* Render pass
	*
	***********************************************************************************/
	auto ms_attachment =		RenderPass::create_multisample_attachment(vk::Format::eB8G8R8A8Unorm, 0, msaa);
	auto resolve_attachment =	RenderPass::create_color_attachment(vk::Format::eB8G8R8A8Unorm, 1);
	auto depth_attachment =		RenderPass::create_depth_stencil_attachment(vk::Format::eD32SfloatS8Uint, 2, msaa);
	
	vk::SubpassDescription subpass_description = {};
	subpass_description.colorAttachmentCount = 1;
	subpass_description.pColorAttachments = &ms_attachment.second;
	subpass_description.pDepthStencilAttachment = &depth_attachment.second;
	subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass_description.pResolveAttachments = &resolve_attachment.second;

	auto render_pass_options = RenderPass::Options()
		.attachment_descriptions({ ms_attachment.first, resolve_attachment.first, depth_attachment.first })
		.attachment_references({ ms_attachment.second, resolve_attachment.second, depth_attachment.second })
		.subpass_descriptions({ subpass_description })
		.subpass_dependencies({ RenderPass::create_default_subpass_dependency() });

	auto render_pass = RenderPass::create(device, render_pass_options);

   /***********************************************************************************
	*
	* Geometry, buffers, and pipeline
	*
	***********************************************************************************/
	auto geometry = geo::Grid();
	auto vbo_0 =	Buffer::create(device, vk::BufferUsageFlagBits::eVertexBuffer, geometry.get_positions(), { Device::QueueType::GRAPHICS });
	auto vbo_1 =	Buffer::create(device, vk::BufferUsageFlagBits::eVertexBuffer, geometry.get_texture_coordinates(), { Device::QueueType::GRAPHICS });
	auto ibo =		Buffer::create(device, vk::BufferUsageFlagBits::eIndexBuffer, geometry.get_indices(), { Device::QueueType::GRAPHICS });
	auto ubo =		Buffer::create(device, vk::BufferUsageFlagBits::eUniformBuffer, sizeof(UniformBufferData), nullptr, { Device::QueueType::GRAPHICS });
	
	UniformBufferData ubo_data = 
	{
		glm::mat4(),
		glm::lookAt({ 0.0f, 0.0, 2.0f }, { 0.0f, 0.0, 0.0f }, glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::perspective(45.0f, static_cast<float>(width / height), 0.1f, 1000.0f) 
	};

	void *data = ubo->get_device_memory()->map(0, ubo->get_device_memory()->get_allocation_size());
	memcpy(data, &ubo_data, sizeof(ubo_data));
	ubo->get_device_memory()->unmap();

	vk::VertexInputBindingDescription binding_0 = { 0, sizeof(float) * 3 }; // input rate vertex: 3 floats between each vertex
	vk::VertexInputBindingDescription binding_1 = { 1, sizeof(float) * 2 }; // input rate vertex: 3 floats between each vertex
	std::vector<vk::VertexInputAttributeDescription> attrs = geometry.get_vertex_input_attribute_descriptions();
	
	auto v_shader = ShaderModule::create(device, ResourceManager::load_file("../assets/shaders/vert.spv"));
	auto f_shader = ShaderModule::create(device, ResourceManager::load_file("../assets/shaders/frag.spv"));

	auto pipeline_options = Pipeline::Options()
		.vertex_input_binding_descriptions({ binding_0, binding_1 })
		.vertex_input_attribute_descriptions(attrs)
		.viewports({ window->get_fullscreen_viewport() })
		.scissors({ window->get_fullscreen_scissor_rect2d() })
		.attach_shader_stages({ v_shader, f_shader })
		.primitive_topology(geometry.get_topology())
		.cull_mode(vk::CullModeFlagBits::eNone)
		.depth_test()
		.samples(msaa)
		.min_sample_shading(0.25f);
	auto pipeline = Pipeline::create(device, render_pass, pipeline_options);

   /***********************************************************************************
	*
	* Images, image views, and samplers
	*
	***********************************************************************************/
	auto image_ms = Image::create(device,
		vk::ImageType::e2D,
		vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
		swapchain->get_image_format(),
		{ width, height, 1 }, 1,
		vk::ImageTiling::eOptimal,
		msaa);
	auto image_ms_view = ImageView::create(device, image_ms);

	auto image_depth = Image::create(device, 
		vk::ImageType::e2D, 
		vk::ImageUsageFlagBits::eDepthStencilAttachment, 
		device->get_supported_depth_format(), 
		{ width, height, 1 }, 1,
		vk::ImageTiling::eOptimal, 
		msaa);
	auto image_depth_view = ImageView::create(device, image_depth);
	
	auto image_sdf_map = Image::create(device,
		vk::ImageType::e3D,
		vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
		swapchain->get_image_format(),
		{ width, height, 32 }, 1,
		vk::ImageTiling::eOptimal);
	auto image_sdf_map_view = ImageView::create(device, image_sdf_map);

	auto sampler = Sampler::create(device);

   /***********************************************************************************
	*
	* Command pool and command buffers
	*
	***********************************************************************************/
	auto command_pool = CommandPool::create(device, device->get_queue_family_index(Device::QueueType::GRAPHICS));

	auto temp_command_buffer = CommandBuffer::create(device, command_pool);

	{
		ScopedRecord scoped(temp_command_buffer);
		temp_command_buffer->transition_image_layout(image_depth, image_depth->get_current_layout(), vk::ImageLayout::eDepthStencilAttachmentOptimal);
	}

	device->one_time_submit(Device::QueueType::GRAPHICS, temp_command_buffer);

	temp_command_buffer->reset();

	{
		ScopedRecord scoped(temp_command_buffer);
		temp_command_buffer->transition_image_layout(image_sdf_map, image_sdf_map->get_current_layout(), vk::ImageLayout::eGeneral);
		temp_command_buffer->clear_color_image(image_sdf_map, clear::RED);
	}

	device->one_time_submit(Device::QueueType::GRAPHICS, temp_command_buffer);

   /***********************************************************************************
	*
	* Framebuffer
	*
	***********************************************************************************/
	std::vector<FramebufferRef> framebuffers(swapchain_image_views.size());
	for (size_t i = 0; i < swapchain_image_views.size(); ++i)
	{
		std::vector<vk::ImageView> image_views = { 
			image_ms_view->get_handle(),			// attachment 0: multisample color 
			swapchain_image_views[i],				// attachment 1: resolve color (swapchain)
			image_depth_view->get_handle()			// attachment 2: depth-stencil
		};

		framebuffers[i] = Framebuffer::create(device, render_pass, image_views, width, height);
	}

   /***********************************************************************************
	*
	* Descriptor pools, descriptor set layouts, and descriptor sets
	*
	***********************************************************************************/
	auto descriptor_pool = DescriptorPool::create(device, { 
		{ vk::DescriptorType::eUniformBuffer, 1 },
		{ vk::DescriptorType::eCombinedImageSampler, 1 }
	});

	// TODO: write a method in the DescriptorPool class that allocates a descriptor set from 
	// a LayoutBuilder and keeps track of the number of active sets / per-descriptor resources
	auto layout_builder = LayoutBuilder::create(device);
	layout_builder->add_next_binding(vk::DescriptorType::eUniformBuffer);
	layout_builder->add_next_binding(vk::DescriptorType::eCombinedImageSampler);
	vk::DescriptorSetLayout layout = layout_builder->build_layout();

	vk::DescriptorSetAllocateInfo descriptor_set_allocate_info = { 
		descriptor_pool->get_handle(),	// descriptor pool
		1,								// number of sets to allocate
		&layout							// descriptor set layout
	};
	vk::DescriptorSet descriptor_set = device->get_handle().allocateDescriptorSets(descriptor_set_allocate_info)[0];

	auto d_buffer_info = ubo->build_descriptor_info();				
	auto d_sampler_info = image_sdf_map_view->build_descriptor_info(sampler);
	vk::WriteDescriptorSet w_descriptor_set_buffer =	{ descriptor_set, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &d_buffer_info };		
	vk::WriteDescriptorSet w_descriptor_set_sampler =	{ descriptor_set, 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &d_sampler_info, nullptr};
	std::vector<vk::WriteDescriptorSet> w_descriptor_sets = { w_descriptor_set_buffer, w_descriptor_set_sampler };

	device->get_handle().updateDescriptorSets(w_descriptor_sets, {});

   /***********************************************************************************
	*
	* Render loop
	*
	***********************************************************************************/
	auto image_available_sem = Semaphore::create(device);
	auto render_complete_sem = Semaphore::create(device);

	while (!window->should_close())
	{
		// Check the windowing system for any user interaction.
		window->poll_events();
		
		// Get the index of the next available image.
		uint32_t image_index = swapchain->acquire_next_swapchain_image(image_available_sem);

		// Set the clear values for each of this framebuffer's attachments:
		// 1. multisample color attachment
		// 2. resolve color attachment
		// 3. depth/stencil attachment
		std::vector<vk::ClearValue> clear_vals = { { clear::BLACK }, { clear::BLACK }, { clear::DEPTH_ONE } };

		// Set up a new command buffer and record draw calls.
		auto command_buffer = CommandBuffer::create(device, command_pool);
		auto command_buffer_handle = command_buffer->get_handle();
		{
			ScopedRecord record(command_buffer);
			command_buffer->begin_render_pass(render_pass, framebuffers[image_index], clear_vals);
			command_buffer->bind_pipeline(pipeline);
			command_buffer->bind_vertex_buffers({ vbo_0, vbo_1 });
			command_buffer->bind_index_buffer(ibo);
			command_buffer->update_push_constant_ranges(pipeline, "time", get_elapsed_seconds());
			command_buffer->bind_descriptor_sets(pipeline, 0, descriptor_set);
			command_buffer->draw_indexed(static_cast<uint32_t>(geometry.get_indices().size()), 1, 0, 0, 0);
			command_buffer->end_render_pass();
		}
		device->submit_with_semaphores(Device::QueueType::GRAPHICS, command_buffer, image_available_sem, render_complete_sem);

		// Wait for rendering to finish before attempting to present...
		device->wait_idle_queue(Device::QueueType::GRAPHICS);
		
		// Present the rendered image to the swapchain.
		device->present(swapchain, image_index, render_complete_sem);
	}

	return 0;
}
