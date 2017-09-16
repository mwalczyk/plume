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

namespace geom
{

	static const VertexAttributeSet active_attributes =
	{
		VertexAttribute::ATTRIBUTE_POSITION,
		VertexAttribute::ATTRIBUTE_COLOR,
		VertexAttribute::ATTRIBUTE_NORMAL,
		VertexAttribute::ATTRIBUTE_TEXTURE_COORDINATES
	};

	vk::Format Geometry::get_vertex_attribute_format(VertexAttribute attribute)
	{
		switch (attribute)
		{
		case VertexAttribute::ATTRIBUTE_POSITION:
		case VertexAttribute::ATTRIBUTE_COLOR:
		case VertexAttribute::ATTRIBUTE_NORMAL: return vk::Format::eR32G32B32Sfloat;
		case VertexAttribute::ATTRIBUTE_TEXTURE_COORDINATES: return vk::Format::eR32G32Sfloat;
		}
	}

	uint32_t Geometry::get_vertex_attribute_dimensions(VertexAttribute attribute)
	{
		switch (attribute)
		{
		case VertexAttribute::ATTRIBUTE_POSITION:
		case VertexAttribute::ATTRIBUTE_COLOR:
		case VertexAttribute::ATTRIBUTE_NORMAL: return 3;
		case VertexAttribute::ATTRIBUTE_TEXTURE_COORDINATES: return 2;
		}
	}

	uint32_t Geometry::get_vertex_attribute_size(VertexAttribute attribute)
	{
		return get_vertex_attribute_dimensions(attribute) * sizeof(float);
	}

	uint32_t Geometry::get_vertex_attribute_offset(VertexAttribute attribute)
	{
		switch (attribute)
		{
		case VertexAttribute::ATTRIBUTE_POSITION: return 0;
		case VertexAttribute::ATTRIBUTE_COLOR: return sizeof(float) * 3;
		case VertexAttribute::ATTRIBUTE_NORMAL: return sizeof(float) * 6;
		case VertexAttribute::ATTRIBUTE_TEXTURE_COORDINATES: return sizeof(float) * 9;
		}
	}

	std::vector<vk::VertexInputAttributeDescription> Geometry::get_vertex_input_attribute_descriptions(uint32_t start_binding, AttributeMode mode)
	{
		std::vector<vk::VertexInputAttributeDescription> input_attribute_descriptions;			
		uint32_t attribute_binding = start_binding;		// The binding number which this attribute takes its data from.
		uint32_t attribute_offset = 0;					// The byte offset of this attribute relative to the start of an element in the vertex input binding

		for (auto available_attribute : active_attributes)
		{
			uint32_t attribute_location = static_cast<uint32_t>(available_attribute);		// The shader binding location number for this attribute.
			vk::Format attribute_format = get_vertex_attribute_format(available_attribute);	// The size and type of the vertex attribute data.

			// If the vertex attributes are interleaved, we need to do some work to figure out
			// the byte offset of each attribute relative to the start of an element (vertex or instance).
			if (mode == AttributeMode::MODE_INTERLEAVED)
			{
				// TODO: use `offsetof(...)` and some sort vertex struct to organize this.
				attribute_offset = get_vertex_attribute_offset(available_attribute);
			}

			input_attribute_descriptions.push_back({ 
				attribute_location, 
				attribute_binding, 
				attribute_format, 
				attribute_offset
			});

			// If the vertex attributes are separate, then we increment the binding index. 
			if (mode == AttributeMode::MODE_SEPARATE)
			{
				attribute_binding++;
			}
		}
		return input_attribute_descriptions;
	}

	std::vector<vk::VertexInputBindingDescription> Geometry::get_vertex_input_binding_descriptions(uint32_t start_binding, AttributeMode mode)
	{
		// TOOD: this does not take into account any custom attributes that might be instanced, i.e. vk::VertexInputRate::eInstance.
		// Also, it doesn't let you create vertex bindings that are not sequential (i.e. 0, 2, 4, 6, etc...), although I'm not sure
		// this actually matters.

		std::vector<vk::VertexInputBindingDescription> binding_descriptions;	
		uint32_t binding_index = start_binding;

		// If the attributes are interleaved (and assuming we are not instancing), then the vertex
		// data will all belong to a single buffer with a stride equal to the total size of all
		// vertex attributes combined.
		if (mode == AttributeMode::MODE_INTERLEAVED)
		{
			uint32_t binding_stride = 0;
			for (const auto& available_attribute : active_attributes)
			{
				binding_stride += get_vertex_attribute_size(available_attribute);
			}

			binding_descriptions.push_back({ 
				binding_index, 
				binding_stride, 
				vk::VertexInputRate::eVertex 
			});

			return binding_descriptions;
		}

		// Otherwise, the attributes are separate and will each exist in a unique buffer (or region
		// of buffer memory).
		for (auto available_attribute : active_attributes)
		{
			uint32_t binding_stride = get_vertex_attribute_size(available_attribute);

			binding_descriptions.push_back({ 
				binding_index, 
				binding_stride, 
				vk::VertexInputRate::eVertex 
			});

			if (mode == AttributeMode::MODE_SEPARATE)
			{
				binding_index++;
			}
		}
		return binding_descriptions;
	}

	std::vector<float> Geometry::get_packed_vertex_attributes()
	{
		// TOOD: need to check if all attribute vectors have the same size.

		std::vector<float> packed_vertex_attributes;

		for (size_t i = 0; i < m_positions.size(); ++i)
		{
			packed_vertex_attributes.push_back(m_positions[i].x);
			packed_vertex_attributes.push_back(m_positions[i].y);
			packed_vertex_attributes.push_back(m_positions[i].z);

			packed_vertex_attributes.push_back(m_colors[i].x);
			packed_vertex_attributes.push_back(m_colors[i].y);
			packed_vertex_attributes.push_back(m_colors[i].z);

			packed_vertex_attributes.push_back(m_normals[i].x);
			packed_vertex_attributes.push_back(m_normals[i].y);
			packed_vertex_attributes.push_back(m_normals[i].z);

			packed_vertex_attributes.push_back(m_texture_coordinates[i].x);
			packed_vertex_attributes.push_back(m_texture_coordinates[i].y);
		}

		return packed_vertex_attributes;
	}

	float* Geometry::get_vertex_attribute_data_ptr(VertexAttribute attribute)
	{
		switch (attribute)
		{
		case VertexAttribute::ATTRIBUTE_POSITION: return reinterpret_cast<float*>(m_positions.data());
		case VertexAttribute::ATTRIBUTE_COLOR: return reinterpret_cast<float*>(m_colors.data());
		case VertexAttribute::ATTRIBUTE_NORMAL: return reinterpret_cast<float*>(m_normals.data());
		case VertexAttribute::ATTRIBUTE_TEXTURE_COORDINATES: return reinterpret_cast<float*>(m_texture_coordinates.data());
		}
	}

	void Geometry::set_colors(const std::vector<glm::vec3>& colors, const glm::vec3& fill_rest)
	{ 
		m_colors = colors;
		m_colors.resize(get_vertex_count(), fill_rest);
	}

	void Geometry::set_colors_random()
	{
		static std::random_device rand;
		static std::mt19937 mersenne(rand());
		static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

		m_colors.clear();
		m_colors.resize(get_vertex_count());

		std::generate(m_colors.begin(), 
					  m_colors.end(), 
					  [&]() -> glm::vec3 { return{ distribution(mersenne), distribution(mersenne), distribution(mersenne) }; });
	}

	Rect::Rect(float width, float height, const glm::vec3& center)
	{
		m_positions = 
		{
			{ -width, -height, 0.0f },
			{  width, -height, 0.0f },
			{  width,  height, 0.0f },
			{ -width,  height, 0.0f }
		};

		for (auto& pt : m_positions)
		{
			pt += center;
		}

		set_colors_solid({ 1.0f, 1.0f, 1.0f });

		m_normals.resize(get_vertex_count(), { 0.0f, 0.0f, 1.0f });

		m_texture_coordinates = 
		{
			{ 0.0f, 1.0f },
			{ 1.0f, 1.0f },
			{ 1.0f, 0.0f },
			{ 0.0f, 0.0f }
		};

		m_indices = 
		{
			0, 1, 2, 
			2, 3, 0
		};
	}

	void Rect::texture_coordinates(const glm::vec2& ul, const glm::vec2& ur, const glm::vec2& lr, const glm::vec2& ll)
	{
		m_texture_coordinates[0] = ul;
		m_texture_coordinates[1] = ur;
		m_texture_coordinates[2] = lr;
		m_texture_coordinates[3] = ll;
	}

	void Rect::colors(const glm::vec3& ul, const glm::vec3& ur, const glm::vec3& lr, const glm::vec3& ll)
	{
		m_colors[0] = ul;
		m_colors[1] = ur;
		m_colors[2] = lr;
		m_colors[3] = ll;
	}

	Grid::Grid(float width, float height, uint32_t u_subdivisions, uint32_t v_subdivisions, const glm::vec3& center)
	{
		for (size_t row = 0; row < v_subdivisions; ++row)
		{
			for (size_t col = 0; col < u_subdivisions; ++col)
			{
				float u = static_cast<float>(col + 1) / u_subdivisions;
				float v = static_cast<float>(row + 1) / v_subdivisions;

				glm::vec3 pt = { u * 2.0f - 1.0f, 
								 v * 2.0f - 1.0f, 
								 0.0f };
				pt.x *= width;
				pt.y *= height;
				pt += center;

				m_positions.push_back(pt);
				m_texture_coordinates.push_back({ u, v });

				// If `u_divisions` is set to 4, we have:
				// 
				// 0 -- 1 -- 2 -- 3
				// | \  | \  |  \ |
				// 4 -- 5 -- 6 -- 7
				// . . . 
				// .
				// .
				// Note: we assume a clockwise winding pattern.
				
				// We don't need to form any triangles for the last row
				size_t cell = col + u_subdivisions * row;
				if ((row + 1) % v_subdivisions != 0)
				{
					// Form the first triangle (i.e. 0 -> 5 -> 4...).
					if ((col + 1) % u_subdivisions != 0)
					{
						m_indices.push_back(cell);
						m_indices.push_back(cell + u_subdivisions + 1);
						m_indices.push_back(cell + u_subdivisions);
					}

					// Only form this triangle if we aren't on the first (0-th) column.
					if (col % u_subdivisions != 0)
					{
						m_indices.push_back(cell);
						m_indices.push_back(cell + u_subdivisions);
						m_indices.push_back(cell - 1);
					}
				}
			}
		}

		m_normals.resize(get_vertex_count(), { 0.0f, 0.0f, 1.0f });

		set_colors_solid({ 1.0f, 1.0f, 1.0f });
	}

	Circle::Circle(float radius, const glm::vec3& center, uint32_t subdivisions)
	{
		m_positions.push_back(center);
		m_indices.push_back(0);

		float div = (2.0f * M_PI) / subdivisions;
		for (size_t i = 0; i < subdivisions; ++i)
		{
			float c = cosf(div * i) * radius;
			float s = sinf(div * i) * radius;
			auto pt = glm::vec3(c, s, 0.0f);

			m_positions.push_back(pt + center);
			m_normals.push_back({ 0.0f, 0.0f, 1.0f });
			m_indices.push_back(i + 1);
		}

		set_colors_solid({ 1.0f, 1.0f, 1.0f });

		m_indices.push_back(1);

		// TODO: figure out how to calculate uv-coordinates.
		m_texture_coordinates.resize(get_vertex_count(), { 0.0f, 0.0f });
	}

	Sphere::Sphere(float radius, const glm::vec3& center, size_t u_divisions, size_t v_divisions)
	{
		// Calculate vertex positions.
		for (int i = 0; i <= v_divisions; ++i)
		{
			float v = i / static_cast<float>(v_divisions);		// Fraction along the v-axis, 0..1
			float phi = v * glm::pi<float>();					// Vertical angle, 0..pi

			for (int j = 0; j <= u_divisions; ++j)
			{
				float u = j / static_cast<float>(u_divisions);	// Fraction along the u-axis, 0..1
				float theta = u * (glm::pi<float>() * 2);		// Rotational angle, 0..2 * pi

				// Spherical to Cartesian coordinates.
				float x = cosf(theta) * sinf(phi);
				float y = cosf(phi);
				float z = sinf(theta) * sinf(phi);
				auto vertex = glm::vec3(x, y, z) * radius + center;

				m_positions.push_back(vertex);
				m_normals.push_back(glm::normalize(vertex));
			}
		}

		set_colors_solid({ 1.0f, 1.0f, 1.0f });

		// Calculate indices.
		for (int i = 0; i < u_divisions * v_divisions + u_divisions; ++i)
		{
			m_indices.push_back(i);
			m_indices.push_back(i + u_divisions + 1);
			m_indices.push_back(i + u_divisions);

			m_indices.push_back(i + u_divisions + 1);
			m_indices.push_back(i);
			m_indices.push_back(i + 1);
		}

		// TODO: figure out how to calculate uv-coordinates.
		m_texture_coordinates.resize(get_vertex_count(), { 0.0f, 0.0f });
	}

	IcoSphere::IcoSphere(float radius, const glm::vec3& center)
	{
		// See: http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html
		const float t = (1.0f + sqrtf(5.0f)) / 2.0f;

		// Calculate positions.
		m_positions =
		{
			{ -1.0f,  t,     0.0f },
			{  1.0f,  t,     0.0f },
			{ -1.0f, -t,     0.0f },
			{  1.0f, -t,     0.0f },
			{  0.0f, -1.0f,  t },
			{  0.0f,  1.0f,  t },
			{  0.0f, -1.0f, -t },
			{  0.0f,  1.0f, -t },
			{  t,     0.0f, -1.0f },
			{  t,     0.0f,  1.0f },
			{ -t,     0.0f, -1.0f },
			{ -t,     0.0f,  1.0f }
		};

		for (auto &position : m_positions)
		{
			position = glm::normalize(position) * radius + center;
		}

		set_colors_solid({ 1.0f, 1.0f, 1.0f });

		size_t vertex_index = 0;
		m_normals.resize(get_vertex_count());
		std::generate(m_normals.begin(),
			m_normals.end(),
			[&] { return glm::normalize(m_positions[vertex_index]); });

		m_indices =
		{
			0,  11, 5,
			0,  5,  1,
			0,  1,  7,
			0,  7,  10,
			0,  10, 11,
			1,  5,  9,
			5,  11, 4,
			11, 10, 2,
			10, 7,  6,
			7,  1,  8,
			3,  9,  4,
			3,  4,  2,
			3,  2,  6,
			3,  6,  8,
			3,  8,  9,
			4,  9,  5,
			2,  4,  11,
			6,  2,  10,
			8,  6,  7,
			9,  8,  1
		};

		// TODO: figure out how to calculate uv-coordinates.
		m_texture_coordinates.resize(get_vertex_count(), { 0.0f, 0.0f });
	}

} // namespace geo