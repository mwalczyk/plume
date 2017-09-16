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

namespace geom
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

	enum class AttributeMode
	{
		MODE_INTERLEAVED,
		MODE_SEPARATE
	};

	using VertexAttributeSet = std::vector<VertexAttribute>;

	class Geometry
	{
	public:

		static vk::Format get_vertex_attribute_format(VertexAttribute attribute);
		static uint32_t get_vertex_attribute_dimensions(VertexAttribute attribute);
		static uint32_t get_vertex_attribute_size(VertexAttribute attribute);
		static uint32_t get_vertex_attribute_offset(VertexAttribute attribute);
		static std::vector<vk::VertexInputAttributeDescription> get_vertex_input_attribute_descriptions(uint32_t start_binding = 0, AttributeMode mode = AttributeMode::MODE_INTERLEAVED);
		static std::vector<vk::VertexInputBindingDescription> get_vertex_input_binding_descriptions(uint32_t start_binding = 0, AttributeMode mode = AttributeMode::MODE_INTERLEAVED);

		virtual ~Geometry() = default;

		virtual vk::PrimitiveTopology get_topology() const = 0;

		size_t get_vertex_count() const { return m_positions.size(); }
		
		std::vector<float> get_packed_vertex_attributes();
		const std::vector<glm::vec3>& get_positions() const { return m_positions; }
		const std::vector<glm::vec3>& get_colors() const { return m_colors; }
		const std::vector<glm::vec3>& get_normals() const { return m_normals; }
		const std::vector<glm::vec2>& get_texture_coordinates() const { return m_texture_coordinates; }
		const std::vector<uint32_t>& get_indices() const { return m_indices; }

		float* get_vertex_attribute_data_ptr(VertexAttribute attribute);

		size_t num_positions() const { return m_positions.size(); }
		size_t num_colors() const { return m_colors.size(); }
		size_t num_normals() const { return m_normals.size(); }
		size_t num_texture_coordinates() const { return m_texture_coordinates.size(); }
		size_t num_indices() const { return m_indices.size(); }

		void set_colors(const std::vector<glm::vec3>& colors, const glm::vec3& fill_rest = { 1.0f, 1.0f, 1.0f });
		void set_colors_solid(const glm::vec3& color) { m_colors = std::vector<glm::vec3>(get_vertex_count(), color); }
		void set_colors_random();

	protected:

		struct Vertex
		{
			glm::vec3 m_position;
			glm::vec3 m_color;
			glm::vec3 m_normal;
			glm::vec2 m_texture_coordinate;
		};

		std::vector<glm::vec3> m_positions;
		std::vector<glm::vec3> m_colors;
		std::vector<glm::vec3> m_normals;
		std::vector<glm::vec2> m_texture_coordinates;
		std::vector<uint32_t> m_indices;
	};

	class Rect : public Geometry
	{
	public:

		Rect() :
			Rect(1.0f, 1.0f, { 0.0f, 0.0f, 0.0f })
		{};

		Rect(float width, float height, const glm::vec3& center);
		
		//! Set the texture coordinates of each of the four corner points of the rectangle. The corners 
		//! are ordered in a clockwise fashion, beginning with the upper-left.
		void texture_coordinates(const glm::vec2& ul, const glm::vec2& ur, const glm::vec2& lr, const glm::vec2& ll);

		//! Set the colors of each of the four corner points of the rectangle. The corners are ordered 
		//! in a clockwise fashion, beginning with the upper-left.
		void colors(const glm::vec3& ul, const glm::vec3& ur, const glm::vec3& lr, const glm::vec3& ll);

		vk::PrimitiveTopology get_topology() const override { return vk::PrimitiveTopology::eTriangleStrip; }
	};

	class Grid : public Geometry
	{
	public:
		Grid() :
			Grid(1.0f, 1.0f)
		{};

		Grid(float width, float height, uint32_t u_subdivisions = 4, uint32_t v_subdivisions = 4, const glm::vec3& center = { 0.0f, 0.0f, 0.0f });

		vk::PrimitiveTopology get_topology() const override { return vk::PrimitiveTopology::eTriangleStrip; }
	};

	class Circle : public Geometry
	{
	public:

		Circle() : 
			Circle(1.0f, { 0.0f, 0.0f, 0.0f }) 
		{};

		Circle(float radius, const glm::vec3& center, uint32_t subdivisions = 30);
		vk::PrimitiveTopology get_topology() const override { return vk::PrimitiveTopology::eTriangleFan; }
	};

	class Sphere : public Geometry
	{
	public:
		Sphere() : 
			Sphere(1.0f, { 0.0f, 0.0f, 0.0f }) 
		{};
		
		Sphere(float radius, const glm::vec3& center, size_t u_divisions = 30, size_t v_divisions = 30);
		
		vk::PrimitiveTopology get_topology() const override { return vk::PrimitiveTopology::eTriangleStrip; }
	};

	class IcoSphere : public Geometry
	{
	public:
		IcoSphere() :
			IcoSphere(1.0f, { 0.0f, 0.0f, 0.0f })
		{};

		IcoSphere(float radius, const glm::vec3& center);

		vk::PrimitiveTopology get_topology() const override { return vk::PrimitiveTopology::eTriangleList; }
	};

} // namespace geo