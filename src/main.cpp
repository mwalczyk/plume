#include "Vk.h"
#include "Geometry.h"

#include "gtc/matrix_transform.hpp"

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

void update_buffer_data()
{

}

UniformBufferData ubo_data;

using namespace graphics;

static const uint32_t width = 800;
static const uint32_t height = 800;
static const uint32_t msaa = 8;
const std::string base_shader_path = "shaders/";

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

	auto swapchain = Swapchain::create(device, surface, width, height);
	auto swapchain_image_views = swapchain->get_image_view_handles();

   /***********************************************************************************
	*
	* Render pass
	*
	***********************************************************************************/
	auto render_pass_builder = RenderPassBuilder::create();
	render_pass_builder->add_color_transient_attachment("color_inter", vk::Format::eB8G8R8A8Unorm, msaa);
	render_pass_builder->add_color_present_attachment("color_final", vk::Format::eB8G8R8A8Unorm);
	render_pass_builder->add_depth_stencil_attachment("depth", vk::Format::eD32SfloatS8Uint, msaa);

	render_pass_builder->begin_subpass_record();
	render_pass_builder->append_attachment_to_subpass("color_inter", AttachmentCategory::CATEGORY_COLOR);
	render_pass_builder->append_attachment_to_subpass("color_final", AttachmentCategory::CATEGORY_RESOLVE);
	render_pass_builder->append_attachment_to_subpass("depth", AttachmentCategory::CATEGORY_DEPTH_STENCIL);
	render_pass_builder->end_subpass_record();

	auto render_pass = RenderPass::create(device, render_pass_builder);

   /***********************************************************************************
	*
	* Geometry, buffers, and pipeline
	*
	***********************************************************************************/ 
	auto geometry = geom::Sphere(1.0f, { 0.0f, 0.0f, 0.0f }, 6, 6);	// could be: geom::Grid(1.0f, 1.0f, 24, 24);
	auto vbo = Buffer::create(device, vk::BufferUsageFlagBits::eVertexBuffer, geometry.get_packed_vertex_attributes());
	auto ibo = Buffer::create(device, vk::BufferUsageFlagBits::eIndexBuffer, geometry.get_indices());
	auto ubo = Buffer::create(device, vk::BufferUsageFlagBits::eUniformBuffer, sizeof(UniformBufferData), nullptr);

	ubo_data =
	{
		glm::mat4(1.0f),
		glm::lookAt({ 0.0f, 0.0, 3.0f },{ 0.0f, 0.0, 0.0f }, glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::perspective(45.0f, window->get_aspect_ratio(), 0.1f, 1000.0f)
	};
	ubo->upload_immediately(&ubo_data);

	std::vector<vk::VertexInputBindingDescription> binds = geometry.get_vertex_input_binding_descriptions();
	std::vector<vk::VertexInputAttributeDescription> attrs = geometry.get_vertex_input_attribute_descriptions();
	
	auto v_shader = ShaderModule::create(device, ResourceManager::load_file(base_shader_path + "shader_vert.spv"));
	auto tc_shader = ShaderModule::create(device, ResourceManager::load_file(base_shader_path + "shader_tesc.spv"));
	auto te_shader = ShaderModule::create(device, ResourceManager::load_file(base_shader_path + "shader_tese.spv"));
	auto f_shader = ShaderModule::create(device, ResourceManager::load_file(base_shader_path + "shader_frag.spv"));
	
	auto pipeline_options = GraphicsPipeline::Options()
		.vertex_input_binding_descriptions(binds)
		.vertex_input_attribute_descriptions(attrs)
		.viewports({ window->get_fullscreen_viewport() })
		.scissors({ window->get_fullscreen_scissor_rect2d() })
		.attach_shader_stages({ v_shader, tc_shader, te_shader, f_shader })
		.primitive_topology(vk::PrimitiveTopology::ePatchList) // could be: primitive_topology(geometry.get_topology())
		.cull_mode(vk::CullModeFlagBits::eNone)
		.enable_depth_test()
		.samples(msaa)
		.wireframe();
	auto pipeline = GraphicsPipeline::create(device, render_pass, pipeline_options);

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
	auto command_pool = CommandPool::create(device, QueueType::GRAPHICS);
	auto temp_command_buffer = CommandBuffer::create(device, command_pool);

	{
		ScopedRecord scoped(temp_command_buffer);
		temp_command_buffer->transition_image_layout(image_depth, image_depth->get_current_layout(), vk::ImageLayout::eDepthStencilAttachmentOptimal);
	}

	device->one_time_submit(QueueType::GRAPHICS, temp_command_buffer);

	temp_command_buffer->reset();

	{
		ScopedRecord scoped(temp_command_buffer);
		temp_command_buffer->transition_image_layout(image_sdf_map, image_sdf_map->get_current_layout(), vk::ImageLayout::eGeneral);
		temp_command_buffer->clear_color_image(image_sdf_map, clear::RED);
	}

	device->one_time_submit(QueueType::GRAPHICS, temp_command_buffer);

   /***********************************************************************************
	*
	* Framebuffer
	*
	***********************************************************************************/
	std::vector<FramebufferRef> framebuffers(swapchain_image_views.size());
	for (size_t i = 0; i < swapchain_image_views.size(); ++i)
	{
		std::map<std::string, vk::ImageView> name_to_image_view_map = 
		{ 
			{ "color_inter", image_ms_view->get_handle() },	// attachment 0: color (multisample / transient)
			{ "color_final", swapchain_image_views[i] },	// attachment 1: color (resolve / present)
			{ "depth", image_depth_view->get_handle() }		// attachment 2: depth-stencil
		};

		framebuffers[i] = Framebuffer::create(device, render_pass, name_to_image_view_map, width, height);
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

	const uint32_t set_id = 0;
	const uint32_t binding_id_ubo = 0;
	const uint32_t binding_id_cis = 1;
	auto layout_builder = DescriptorSetLayoutBuilder::create(device);
	layout_builder->begin_descriptor_set_record(set_id);				// BEG set 0
	layout_builder->add_ubo(binding_id_ubo);							// --- binding 0
	layout_builder->add_cis(binding_id_cis);							// --- binding 1
	layout_builder->end_descriptor_set_record();						// END set 0

	vk::DescriptorSet descriptor_set = descriptor_pool->allocate_descriptor_sets(layout_builder, { set_id })[0];

	auto d_buffer_info = ubo->build_descriptor_info();				
	auto d_sampler_info = image_sdf_map_view->build_descriptor_info(sampler);
	vk::WriteDescriptorSet w_descriptor_set_buffer =	{ descriptor_set, binding_id_ubo, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &d_buffer_info };
	vk::WriteDescriptorSet w_descriptor_set_sampler =	{ descriptor_set, binding_id_cis, 0, 1, vk::DescriptorType::eCombinedImageSampler, &d_sampler_info, nullptr};
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

		// Update buffer data.
		float angle = get_elapsed_seconds();
		ubo_data.model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3{ 0.0f, 1.0f, 0.0f });
		ubo_data.model = glm::rotate(ubo_data.model, angle * 0.5f, glm::vec3{ 1.0f, 0.0f, 0.0f });
		ubo->upload_immediately(&ubo_data);
		
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
			command_buffer->bind_vertex_buffers({ vbo });
			command_buffer->bind_index_buffer(ibo);
			command_buffer->update_push_constant_ranges(pipeline, "time", get_elapsed_seconds());
			command_buffer->update_push_constant_ranges(pipeline, "mouse", window->get_mouse_position());
			command_buffer->bind_descriptor_sets(pipeline, 0, { descriptor_set });
			command_buffer->draw_indexed(static_cast<uint32_t>(geometry.num_indices()));
			command_buffer->end_render_pass();
		}
		device->submit_with_semaphores(QueueType::GRAPHICS, command_buffer, { image_available_sem }, { render_complete_sem });
		
		device->wait_idle_queue(QueueType::GRAPHICS);

		// Present the rendered image to the swapchain.
		device->present(swapchain, image_index, { render_complete_sem });
	}

	return 0;
}
