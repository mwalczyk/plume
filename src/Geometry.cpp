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

#include "Geometry.h"

namespace geo
{
	vk::Format attribute_to_format(VertexAttribute attribute)
	{
		switch (attribute)
		{
		case VertexAttribute::ATTRIBUTE_POSITION:
		case VertexAttribute::ATTRIBUTE_COLOR:
		case VertexAttribute::ATTRIBUTE_NORMAL:
			return vk::Format::eR32G32B32Sfloat;
		case VertexAttribute::ATTRIBUTE_TEXTURE_COORDINATES:
			return vk::Format::eR32G32Sfloat;
		default:
			return vk::Format::eR32G32Sfloat;
		}
	}

	float* Geometry::get_vertex_attribute(VertexAttribute attribute) 
	{
		switch (attribute)
		{
		case geo::VertexAttribute::ATTRIBUTE_POSITION: return reinterpret_cast<float*>(m_positions.data());
		case geo::VertexAttribute::ATTRIBUTE_COLOR: return reinterpret_cast<float*>(m_colors.data());
		case geo::VertexAttribute::ATTRIBUTE_NORMAL: return reinterpret_cast<float*>(m_normals.data());
		case geo::VertexAttribute::ATTRIBUTE_TEXTURE_COORDINATES: return reinterpret_cast<float*>(m_texture_coordinates.data());
		}
	}

	void Geometry::set_random_colors()
	{
		static std::random_device rand;
		static std::mt19937 mersenne(rand());
		static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

		m_colors.clear();
		m_colors.resize(get_vertex_count());

		std::generate(m_colors.begin(), m_colors.end(), [&]() -> glm::vec3 { return{ distribution(mersenne), distribution(mersenne), distribution(mersenne) }; });
	}

	size_t Geometry::get_vertex_attribute_dimensions(VertexAttribute attribute) const
	{
		switch (attribute)
		{
		case geo::VertexAttribute::ATTRIBUTE_POSITION: return 3;
		case geo::VertexAttribute::ATTRIBUTE_COLOR: return 3;
		case geo::VertexAttribute::ATTRIBUTE_NORMAL: return 3;
		case geo::VertexAttribute::ATTRIBUTE_TEXTURE_COORDINATES: return 2;
		}
	}

	std::vector<vk::VertexInputAttributeDescription> Geometry::get_vertex_input_attribute_descriptions(uint32_t start_binding, AttributeMode mode)
	{
		static const VertexAttributeSet active_attributes =
		{
			VertexAttribute::ATTRIBUTE_POSITION,
			VertexAttribute::ATTRIBUTE_COLOR,
			VertexAttribute::ATTRIBUTE_NORMAL,
			VertexAttribute::ATTRIBUTE_TEXTURE_COORDINATES
		};

		std::vector<vk::VertexInputAttributeDescription> input_attribute_descriptions;
		uint32_t attribute_location = 0;
		uint32_t attribute_binding = start_binding;

		for (const auto& available_attribute : active_attributes)
		{
			vk::Format attribute_format = attribute_to_format(available_attribute);

			input_attribute_descriptions.push_back({ attribute_location, attribute_binding, attribute_format });

			if (mode == AttributeMode::MODE_SEPARATE)
			{
				attribute_binding++;
			}
			attribute_location++;
		}
		return input_attribute_descriptions;
	}

	IcoSphere::IcoSphere()
	{
		// See: http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html
		float t = (1.0f + sqrtf(5.0f)) / 2.0f;
		
		// Calculate positions.
		m_positions = {
			{-1.0f,  t,     0.0f},
			{ 1.0f,  t,     0.0f},
			{-1.0f, -t,     0.0f},
			{ 1.0f, -t,     0.0f},
			{ 0.0f, -1.0f,  t},
			{ 0.0f,  1.0f,  t},
			{ 0.0f, -1.0f, -t},
			{ 0.0f,  1.0f, -t},
			{ t,     0.0f, -1.0f},
			{ t,     0.0f,  1.0f},
			{-t,     0.0f, -1.0f},
			{-t,     0.0f,  1.0f}
		};

		for (auto &position : m_positions)
		{
			position = glm::normalize(position);
		}

		// Set the default color.
		set_solid({ 1.0f, 1.0f, 1.0f });

		// Calculate indices.
		m_indices = {
			0, 11, 5,
			0, 5, 1,
			0, 1, 7,
			0, 7, 10,
			0, 10, 11,
			1, 5, 9,
			5, 11, 4,
			11, 10, 2,
			10, 7, 6,
			7, 1, 8,
			3, 9, 4,
			3, 4, 2,
			3, 2, 6,
			3, 6, 8,
			3, 8, 9,
			4, 9, 5,
			2, 4, 11, 
			6, 2, 10,
			8, 6, 7,
			9, 8, 1
		};
	}

	Grid::Grid()
	{
		m_positions = {
			{ -1.0f, -1.0f, 0.0f },
			{  1.0f, -1.0f,	0.0f },
			{  1.0f, 1.0f,	0.0f },
			{ -1.0f, 1.0f,	0.0f }
		};

		// Set the default color.
		set_solid({ 1.0f, 1.0f, 1.0f });

		m_texture_coordinates = {
			{ 0.0f, 1.0f },
			{ 1.0f, 1.0f },
			{ 1.0f, 0.0f },
			{ 0.0f, 0.0f }
		};

		m_indices = {
			0, 1, 2, 
			2, 3, 0
		};
	}

	Circle::Circle(float radius, uint32_t subdivisions)
	{
		m_positions.push_back({ 0.0f, 0.0f, -2.0f });
		m_indices.push_back(0);

		float div = (2.0f * M_PI) / subdivisions;
		for (size_t i = 0; i < subdivisions; ++i)
		{
			float c = cosf(div * i) * radius;
			float s = sinf(div * i) * radius;
			m_positions.push_back({ c, s, -2.0f });
			m_indices.push_back(i + 1);
		}

		m_indices.push_back(1);

		// Set the default color.
		set_solid({ 1.0f, 1.0f, 1.0f });
	}

	Sphere::Sphere()
	{
		const uint32_t v_divisions = 60;
		const uint32_t u_divisions = 60;
		const float radius = 1.0f;

		// Calculate vertex positions
		for (int i = 0; i <= v_divisions; ++i)
		{
			float v = i / static_cast<float>(v_divisions);		// Fraction along the v-axis, 0..1
			float phi = v * glm::pi<float>();					// Vertical angle, 0..pi

			for (int j = 0; j <= u_divisions; ++j)
			{
				float u = j / static_cast<float>(u_divisions);	// Fraction along the u-axis, 0..1
				float theta = u * (glm::pi<float>() * 2);		// Rotational angle, 0..2*pi

				// Spherical to Cartesian coordinates
				float x = cosf(theta) * sinf(phi);
				float y = cosf(phi);
				float z = sinf(theta) * sinf(phi);
				auto vertex = glm::vec3(x, y, z) * radius;

				m_positions.push_back(vertex);
				m_normals.push_back(glm::normalize(vertex));
			}
		}

		// Calculate indices
		for (int i = 0; i < u_divisions * v_divisions + u_divisions; ++i)
		{
			m_indices.push_back(i);
			m_indices.push_back(i + u_divisions + 1);
			m_indices.push_back(i + u_divisions);

			m_indices.push_back(i + u_divisions + 1);
			m_indices.push_back(i);
			m_indices.push_back(i + 1);
		}

		set_solid({ 1.0f, 1.0f, 1.0f });
	}

} // namespace geo