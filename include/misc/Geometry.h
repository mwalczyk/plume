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

#pragma once

#define _USE_MATH_DEFINES
#include <vector>
#include <iostream>
#include <algorithm>
#include <functional>
#include <random>
#include <math.h>

#include "glm.hpp"
#include "gtc/type_ptr.hpp"

#include "Platform.h"

namespace geo
{

	enum class VertexAttribute
	{
		ATTRIBUTE_POSITION,
		ATTRIBUTE_COLOR,
		ATTRIBUTE_NORMAL,
		ATTRIBUTE_TEXTURE_COORDINATES,
		ATTRIBUTE_CUSTOM_0,
		ATTRIBUTE_CUSTOM_1,
		ATTRIBUTE_CUSTOM_2,
		ATTRIBUTE_CUSTOM_3
	};

	using VertexAttributeSet = std::vector<VertexAttribute>;

	enum class AttributeMode
	{
		MODE_INTERLEAVED,
		MODE_SEPARATE
	};

	class Geometry
	{
	public:

		virtual ~Geometry() = default;

		virtual vk::PrimitiveTopology get_topology() const = 0;
		virtual uint32_t get_vertex_attribute_dimensions(VertexAttribute attribute) const;
		virtual uint32_t get_vertex_attribute_size(VertexAttribute attribute) const;
		//virtual VertexAttributeSet get_available_attributes() const = 0;

		inline size_t get_vertex_count() const { return m_positions.size(); }

		inline const std::vector<glm::vec3>& get_positions() const { return m_positions; }
		inline const std::vector<glm::vec3>& get_colors() const { return m_colors; }
		inline const std::vector<glm::vec3>& get_normals() const { return m_normals; }
		inline const std::vector<glm::vec2>& get_texture_coordinates() const { return m_texture_coordinates; }

		inline size_t num_positions() const { return m_positions.size(); }
		inline size_t num_colors() const { return m_colors.size(); }
		inline size_t num_normals() const { return m_normals.size(); }
		inline size_t num_texture_coordinates() const { return m_texture_coordinates.size(); }

		inline const std::vector<uint32_t>& get_indices() const { return m_indices; }
		inline size_t num_indices() const { return m_indices.size(); }

		std::vector<vk::VertexInputAttributeDescription> get_vertex_input_attribute_descriptions(uint32_t start_binding = 0, AttributeMode mode = AttributeMode::MODE_SEPARATE);
		std::vector<vk::VertexInputBindingDescription> get_vertex_input_binding_descriptions(uint32_t start_binding = 0, AttributeMode mode = AttributeMode::MODE_SEPARATE);

		float* get_vertex_attribute(VertexAttribute attribute);


		inline void set_colors(const std::vector<glm::vec3>& colors) { m_colors = colors; }
		inline void set_solid(const glm::vec3& color) { m_colors = std::vector<glm::vec3>(get_vertex_count(), color); }
		void set_random_colors();

	protected:

		std::vector<glm::vec3> m_positions;
		std::vector<glm::vec3> m_colors;
		std::vector<glm::vec3> m_normals;
		std::vector<glm::vec2> m_texture_coordinates;
		std::vector<uint32_t> m_indices;
	};

	class IcoSphere: public Geometry
	{
	public:

		IcoSphere();

		inline vk::PrimitiveTopology get_topology() const override { return vk::PrimitiveTopology::eTriangleList; }
	};

	class Grid : public Geometry
	{
	public:

		Grid();

		inline vk::PrimitiveTopology get_topology() const override { return vk::PrimitiveTopology::eTriangleStrip; }
	};

	class Circle : public Geometry
	{
	public:

		Circle() : Circle(1.0f) {};
		Circle(float radius, uint32_t subdivisions = 30);

		inline vk::PrimitiveTopology get_topology() const override { return vk::PrimitiveTopology::eTriangleFan; }
	};

	class Sphere : public Geometry
	{
	public:

		Sphere();

		inline vk::PrimitiveTopology get_topology() const override { return vk::PrimitiveTopology::eTriangleStrip; }
	};

} // namespace geo