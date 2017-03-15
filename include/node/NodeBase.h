#pragma once

#include <memory>
#include <vector>

#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/type_ptr.hpp"
#include "glm/glm/gtc/matrix_transform.hpp"

#include "Device.h"
#include "Window.h"
#include "Buffer.h"
#include "DescriptorPool.h"

namespace node
{

	//! To properly render the entire network of nodes, the node manager traverses the network and draws
	//! each node as a rectangle:
	//!
	//! 1) Begin the render pass, which consists of a single subpass involving a framebuffer with one 
	//!    color attachment
	//! 2) Bind the vertex buffer for the rectangle mesh
	//! 3) Bind the index buffer for the rectangle mesh
	//! 4) Bind the top-level descriptor set that contains the transformation matrices for the entire 
	//!	   networke
	//! 5) For each node that is visible in the network editor:
	//!		a. Update the per-node descriptor set that contains the transformation matrices (translation and
	//!		   scale) for the current node
	//!		b. Bind the current node's pipeline derivative
	//!		c. Draw the mesh
	//! 6) For each node connection (wire) that is visible in the network editor: TODO
	//! 7) End the render pass
	//! 8) Submit the command buffer for processing
	class NodeManager
	{
	public:

		NodeManager(const graphics::DeviceRef& device, const graphics::WindowRef& window);

		void prepare_command_buffer();

	private:

		std::vector<NodeBaseRef> m_nodes;
		graphics::DeviceRef m_device;
		graphics::WindowRef m_window;
		graphics::BufferRef m_position_buffer;
		graphics::BufferRef m_index_buffer;
		graphics::BufferRef m_uniform_buffer;
		graphics::DescriptorPoolRef m_descriptor_pool;
		vk::DescriptorSet m_descriptor_set;
		glm::mat4 m_projection_matrix;
	};

	class NodeBase;
	using NodeBaseRef = std::shared_ptr<NodeBase>;

	class NodeBase
	{
	public:

		NodeBase();
		NodeBase(glm::vec2 position, glm::vec2 size);
		
	private:

		glm::vec2 m_position;
		glm::vec2 m_size;
		bool m_dirty;
		bool m_selected;
		std::string m_name;
		std::vector<NodeBaseRef> m_inputs;
		std::vector<NodeBaseRef> m_outputs;
	};

} // namespace node