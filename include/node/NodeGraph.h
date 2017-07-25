#pragma once

#include "NodeBase.h"

namespace node 
{

	class NodeGraph;
	using NodeGraphRef = std::shared_ptr<NodeGraph>;

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

		void record_draw_commands(graphics::CommandBufferRef& command_buffer);

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

} // namespace node