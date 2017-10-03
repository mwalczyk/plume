#pragma once

#include "Vk.h"
#include "Geometry.h"

#include "gtc/matrix_transform.hpp"

struct UniformBufferData
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

UniformBufferData ubo_data;

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
	pl::graphics::Instance instance;
	pl::graphics::Window window{ instance, width, height };
	pl::graphics::Device device{ instance.get_physical_devices()[0], window.get_surface_handle() };
	pl::graphics::Swapchain swapchain{ device, window.get_surface_handle(), width, height };

	auto swapchain_image_views = swapchain.get_image_view_handles();

	/***********************************************************************************
	 *
	 * Render pass
	 *
	 ***********************************************************************************/
	const vk::Format swapchain_format = swapchain.get_image_format();
	const vk::Extent3D fs_extent = { width, height, 1 };

	std::shared_ptr<pl::graphics::RenderPassBuilder> rpb = pl::graphics::RenderPassBuilder::create();
	rpb->add_color_transient_attachment("color_inter", swapchain_format, msaa);	// multisampling
	rpb->add_color_present_attachment("color_final", swapchain_format);			// no multisampling
	rpb->add_depth_stencil_attachment("depth", device.get_supported_depth_format(), msaa);

	rpb->begin_subpass_record();
	rpb->append_attachment_to_subpass("color_inter", pl::graphics::AttachmentCategory::CATEGORY_COLOR);
	rpb->append_attachment_to_subpass("color_final", pl::graphics::AttachmentCategory::CATEGORY_RESOLVE);
	rpb->append_attachment_to_subpass("depth", pl::graphics::AttachmentCategory::CATEGORY_DEPTH_STENCIL);
	rpb->end_subpass_record();

	pl::graphics::RenderPass render_pass{ device, rpb };

	/***********************************************************************************
	 *
	 * Geometry, buffers, and pipeline
	 *
	 ***********************************************************************************/
	pl::geom::Rect geometry = pl::geom::Rect();
	pl::graphics::Buffer vbo{ device, vk::BufferUsageFlagBits::eVertexBuffer, geometry.get_packed_vertex_attributes() };
	pl::graphics::Buffer ibo{ device, vk::BufferUsageFlagBits::eIndexBuffer, geometry.get_indices() };
	pl::graphics::Buffer ubo{ device, vk::BufferUsageFlagBits::eUniformBuffer, sizeof(UniformBufferData), nullptr };

	ubo_data =
	{
		glm::mat4(1.0f),
		glm::lookAt({ 0.0f, 0.0, 3.0f },{ 0.0f, 0.0, 0.0f }, glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::perspective(45.0f, window.get_aspect_ratio(), 0.1f, 1000.0f)
	};
	ubo.upload_immediately(&ubo_data);

	auto binds = geometry.get_vertex_input_binding_descriptions();
	auto attrs = geometry.get_vertex_input_attribute_descriptions();

	auto v_resource = pl::fsys::ResourceManager::load_file(base_shader_path + "raymarch_vert.spv");
	auto f_resource = pl::fsys::ResourceManager::load_file(base_shader_path + "raymarch_frag.spv");
	auto v_shader = pl::graphics::ShaderModule::create(device, v_resource);
	auto f_shader = pl::graphics::ShaderModule::create(device, f_resource);

	auto pipeline_options = pl::graphics::GraphicsPipeline::Options()
							.vertex_input_binding_descriptions(binds)
							.vertex_input_attribute_descriptions(attrs)
							.viewports({ window.get_fullscreen_viewport() })
							.scissors({ window.get_fullscreen_scissor_rect2d() })
							.attach_shader_stages({ v_shader, f_shader })
							.primitive_topology(geometry.get_topology())
							.cull_back()
							.depth_test_enabled()
							.samples(msaa);
	pl::graphics::GraphicsPipeline pipeline{ device, render_pass, pipeline_options };

	/***********************************************************************************
	 *
	 * Images, image views, and samplers
	 *
	 ***********************************************************************************/
	pl::graphics::Image image_ms{ device,
								  vk::ImageType::e2D,
								  vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
								  swapchain_format, fs_extent, 1, 1,
								  vk::ImageTiling::eOptimal, msaa };

	pl::graphics::ImageView image_ms_view{ device, image_ms };

	pl::graphics::Image image_depth{ device,
									 vk::ImageType::e2D,
									 vk::ImageUsageFlagBits::eDepthStencilAttachment,
									 device.get_supported_depth_format(), fs_extent, 1, 1,
									 vk::ImageTiling::eOptimal, msaa };

	pl::graphics::ImageView image_depth_view{ device, image_depth, vk::ImageViewType::e2D, pl::graphics::Image::build_single_layer_subresource(vk::ImageAspectFlagBits::eDepth) };

	pl::graphics::Image image_sdf_map{ device,
									   vk::ImageType::e3D,
									   vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
									   swapchain_format, { width, height, 32 }, 1, 1,
									   vk::ImageTiling::eOptimal };

	pl::graphics::ImageView image_sdf_map_view{ device, image_sdf_map, vk::ImageViewType::e3D };	// by default, ImageView's are 2D.

	pl::graphics::Sampler sampler{ device };

	/***********************************************************************************
	 *
	 * Command pool and command buffers
	 *
	 ***********************************************************************************/
	pl::graphics::CommandPool command_pool{ device, pl::graphics::QueueType::GRAPHICS };
	pl::graphics::CommandBuffer temp_cb{ device, command_pool };

	temp_cb.begin();
	temp_cb.transition_image_layout(image_depth, image_depth.get_current_layout(), vk::ImageLayout::eDepthStencilAttachmentOptimal);
	temp_cb.transition_image_layout(image_sdf_map, image_sdf_map.get_current_layout(), vk::ImageLayout::eGeneral);
	temp_cb.clear_color_image(image_sdf_map, pl::utils::clear_color::red());
	temp_cb.end();
	device.one_time_submit(pl::graphics::QueueType::GRAPHICS, temp_cb);

	/***********************************************************************************
	 *
	 * Framebuffer
	 *
	 ***********************************************************************************/
	std::vector<pl::graphics::Framebuffer> framebuffers;
	for (size_t i = 0; i < swapchain_image_views.size(); ++i)
	{
		std::map<std::string, vk::ImageView> name_to_image_view_map =
		{
			{ "color_inter", image_ms_view.get_handle() },	// attachment 0: color (multisampled)
			{ "color_final", swapchain_image_views[i] },	// attachment 1: color (resolve)
			{ "depth", image_depth_view.get_handle() }		// attachment 2: depth
		};

		framebuffers.emplace_back(pl::graphics::Framebuffer{ device, render_pass, name_to_image_view_map, width, height });
	}

	/***********************************************************************************
	 *
	 * Descriptor pools, descriptor set layouts, and descriptor sets
	 *
	 ***********************************************************************************/
	std::vector<vk::DescriptorPoolSize> pool_sizes = { { vk::DescriptorType::eUniformBuffer, 1 }, 
													   { vk::DescriptorType::eCombinedImageSampler, 1 } };
	pl::graphics::DescriptorPool descriptor_pool{ device, pool_sizes };

	const uint32_t set_id = 0;
	const uint32_t binding_id_ubo = 0;
	const uint32_t binding_id_cis = 1;
	std::shared_ptr<pl::graphics::DescriptorSetLayoutBuilder> dslb = pl::graphics::DescriptorSetLayoutBuilder::create(device);
	dslb->begin_descriptor_set_record(set_id);				// BEG set 0
	dslb->add_ubo(binding_id_ubo);							// --- binding 0
	dslb->add_cis(binding_id_cis);							// --- binding 1
	dslb->end_descriptor_set_record();						// END set 0

	vk::DescriptorSet descriptor_set = descriptor_pool.allocate_descriptor_sets(dslb, { set_id })[0];

	vk::DescriptorBufferInfo dbuff_info = ubo.build_descriptor_info();				
	vk::DescriptorImageInfo dimag_info = image_sdf_map_view.build_descriptor_info(sampler);
	vk::WriteDescriptorSet wds_buff = { descriptor_set, binding_id_ubo, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &dbuff_info };
	vk::WriteDescriptorSet wds_samp = { descriptor_set, binding_id_cis, 0, 1, vk::DescriptorType::eCombinedImageSampler, &dimag_info, nullptr};
	std::vector<vk::WriteDescriptorSet> w_descriptor_sets = { wds_buff, wds_samp };

	device.get_handle().updateDescriptorSets(w_descriptor_sets, {});

   /***********************************************************************************
	*
	* Render loop
	*
	***********************************************************************************/
	pl::graphics::Semaphore image_available_sem{ device };
	pl::graphics::Semaphore render_complete_sem{ device };

	while (!window.should_close())
	{
		// Check the windowing system for any user interaction.
		window.poll_events();
		
		// Get the index of the next available image.
		uint32_t image_index = device.acquire_next_swapchain_image(swapchain, image_available_sem);

		// Set the clear values for each of this framebuffer's attachments:
		// 1. multisample color attachment
		// 2. resolve color attachment
		// 3. depth/stencil attachment
		std::vector<vk::ClearValue> clear_vals = { pl::utils::clear_color::black(),			// color (multisampled)
												   pl::utils::clear_color::black(),			// color (resolve)
												   pl::utils::clear_depth::depth_one() };	// depth
		
		// Set up a new command buffer and record draw calls.
		pl::graphics::CommandBuffer command_buffer{ device, command_pool };
		{
			pl::graphics::ScopedRecord record(command_buffer);
			command_buffer.begin_render_pass(render_pass, framebuffers[image_index], clear_vals);
			command_buffer.bind_pipeline(pipeline);
			command_buffer.bind_vertex_buffer(vbo);
			command_buffer.bind_index_buffer(ibo);
			command_buffer.update_push_constant_ranges(pipeline, "time", pl::utils::app::get_elapsed_seconds());
			command_buffer.update_push_constant_ranges(pipeline, "mouse", window.get_mouse_position(true, true));
			command_buffer.bind_descriptor_sets(pipeline, set_id, { descriptor_set });
			command_buffer.draw_indexed(static_cast<uint32_t>(geometry.num_indices()));
			command_buffer.end_render_pass();
		}
		device.submit_with_semaphores(pl::graphics::QueueType::GRAPHICS, command_buffer, image_available_sem, render_complete_sem);
		
		// Wait for all work on this queue to finish.
		device.wait_idle_queue(pl::graphics::QueueType::GRAPHICS);

		// Present the rendered image to the swapchain.
		device.present(swapchain, image_index, render_complete_sem);
	}

	return 0;
}
