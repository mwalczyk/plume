#include "NodeBase.h"

namespace node
{
	static const std::vector<glm::vec2> positions = 
	{ 
		{ -1.0f, -1.0f },
		{ 1.0f, -1.0f },
		{ 1.0f, 1.0f },
		{ -1.0f, 1.0f }
	};

	static const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };

	NodeManager::NodeManager(const graphics::DeviceRef& device, const graphics::WindowRef& window) :
		m_device(device),
		m_window(window)
	{
		m_projection_matrix = glm::ortho(0.0f, static_cast<float>(m_window->get_width()), static_cast<float>(m_window->get_height()), 0.0f, -1.0f, 1.0f);

		// Setup buffers 
		m_position_buffer = graphics::Buffer::create(m_device, vk::BufferUsageFlagBits::eVertexBuffer, positions);
		m_index_buffer = graphics::Buffer::create(m_device, vk::BufferUsageFlagBits::eIndexBuffer, indices);
		m_uniform_buffer = graphics::Buffer::create(m_device, vk::BufferUsageFlagBits::eUniformBuffer, sizeof(glm::mat4), glm::value_ptr(m_projection_matrix));

		// Setup the descriptor pool
		m_descriptor_pool = graphics::DescriptorPool::create(m_device, { { vk::DescriptorType::eUniformBuffer, 1 } });

		// Setup the layout for the primary descriptor set (network-level descriptors)
		vk::DescriptorSetLayoutBinding descriptor_set_layout_binding;
		descriptor_set_layout_binding.binding = 0;
		descriptor_set_layout_binding.descriptorCount = 1;
		descriptor_set_layout_binding.descriptorType = vk::DescriptorType::eUniformBuffer;
		descriptor_set_layout_binding.pImmutableSamplers = nullptr;
		descriptor_set_layout_binding.stageFlags = vk::ShaderStageFlagBits::eVertex;

		vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info;
		descriptor_set_layout_create_info.bindingCount = 1;
		descriptor_set_layout_create_info.pBindings = &descriptor_set_layout_binding;
		
		vk::DescriptorSetLayout descriptor_set_layout = m_device->get_handle().createDescriptorSetLayout(descriptor_set_layout_create_info);

		// Allocate the descriptor set(s) from the descriptor pool
		vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(m_descriptor_pool->get_handle(), 1, &descriptor_set_layout);
		m_descriptor_set = m_device->get_handle().allocateDescriptorSets(descriptor_set_allocate_info)[0];

		// Update the descriptor set
		auto descriptor_buffer_info = m_uniform_buffer->build_descriptor_info();										
		vk::WriteDescriptorSet w_descriptor_set_buffer = { m_descriptor_set, 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &descriptor_buffer_info };
		std::vector<vk::WriteDescriptorSet> w_descriptor_sets = { w_descriptor_set_buffer };
		m_device->get_handle().updateDescriptorSets(w_descriptor_sets, {});
	}

	void NodeManager::record_draw_commands(graphics::CommandBufferRef& command_buffer)
	{
		command_buffer->bind_vertex_buffers({ m_position_buffer });
		command_buffer->bind_index_buffer(m_index_buffer);
		//command_buffer->get_handle().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline->get_pipeline_layout_handle(), 0, 1, &m_descriptor_set, 0, nullptr);
		command_buffer->draw_indexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
	}

	NodeBase::NodeBase() :
		NodeBase({ 0.0f, 0.0f }, { 50.0f, 30.0f })
	{

	}

	NodeBase::NodeBase(glm::vec2 position, glm::vec2 size) :
		m_position(position),
		m_size(size),
		m_dirty(true),
		m_selected(false),
		m_name("Undefined")
	{

	}
}