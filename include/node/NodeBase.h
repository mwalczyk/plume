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
#include "CommandBuffer.h"

namespace node
{

	class NodeGraph;
	using NodeGraphRef = std::shared_ptr<NodeGraph>;

	class NodeBase;
	using NodeBaseRef = std::shared_ptr<NodeBase>;

	class NodeBase
	{
	public:

		enum class Family
		{
			GENERATOR,			// Box, sphere, torus, etc.
			MODIFIER,			// Repeat, etc.
			OPERATOR,			// Union, intersection, smooth minimum, etc.
			DEFORMATION			// Sinusoidal, noise, etc.
		};

		struct Internals
		{
			glm::vec2 position;	// Position of this node in the network editor
			glm::vec2 size;		// Size of the rectangle used to draw this node
			std::string name;	// Display name of this node
			size_t uuid;		// Unique identifier corresponding to this node
			bool selected;		// Flag indicating whether or not the user has this node selected
		};

		NodeBase();

		NodeBase(const NodeGraphRef& graph, const Internals& internals);

		NodeBase(const NodeBase& other) : 
			NodeBase(other.m_graph, other.m_internals) 
		{ 
			++s_uuid; 
		}

		virtual ~NodeBase();

		//! Pure virtual functions must be overriden by all derived classes: all nodes should
		//! be able to advertise which family they belong to and what shader code they generate.
		virtual Family get_family() const = 0;	
		virtual std::string get_shader_code() const = 0;

		virtual const Internals& get_internals() const final { return m_internals; }
		virtual const std::vector<NodeBaseRef>& get_inputs() const final { return m_inputs; }
		virtual const std::vector<NodeBaseRef>& get_outputs() const final { return m_outputs; }

	protected:

		NodeGraphRef m_graph;
		Internals m_internals;
		std::vector<NodeBaseRef> m_inputs;
		std::vector<NodeBaseRef> m_outputs;

		// TODO: the default name that will be given to this node (should be a combination of
		// the node family, node function, and UUID)
		static const std::string s_default_name_prefix;

		// TODO: include boost::uuid
		static size_t s_uuid;	
	};

	class NodeSDFSphere : public NodeBase
	{
	public:

		virtual Family get_family() const override
		{
			return NodeBase::Family::GENERATOR;
		};
		
		virtual std::string get_shader_code() const = 0;

	private:

	};

} // namespace node