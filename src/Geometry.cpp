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

	float* Geometry::getVertexAttribute(VertexAttribute tAttribute) 
	{
		switch (tAttribute)
		{
		case geo::VertexAttribute::ATTRIBUTE_POSITION: return reinterpret_cast<float*>(mPositions.data());
		case geo::VertexAttribute::ATTRIBUTE_COLOR: return reinterpret_cast<float*>(mColors.data());
		case geo::VertexAttribute::ATTRIBUTE_NORMAL: return reinterpret_cast<float*>(mNormals.data());
		case geo::VertexAttribute::ATTRIBUTE_TEXTURE_COORDINATES: return reinterpret_cast<float*>(mTextureCoordinates.data());
		}
	}

	void Geometry::setRandomColors()
	{
		static std::random_device rand;
		static std::mt19937 mersenne(rand());
		static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

		mColors.clear();
		mColors.resize(getVertexCount());

		std::generate(mColors.begin(), mColors.end(), [&]() -> glm::vec3 { return{ distribution(mersenne), distribution(mersenne), distribution(mersenne) }; });
	}

	size_t Geometry::getVertexAttributeDimensions(VertexAttribute tAttribute) const
	{
		switch (tAttribute)
		{
		case geo::VertexAttribute::ATTRIBUTE_POSITION: return 3;
		case geo::VertexAttribute::ATTRIBUTE_COLOR: return 3;
		case geo::VertexAttribute::ATTRIBUTE_NORMAL: return 3;
		case geo::VertexAttribute::ATTRIBUTE_TEXTURE_COORDINATES: return 2;
		}
	}

	IcoSphere::IcoSphere()
	{
		// See: http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html
		float t = (1.0f + sqrtf(5.0f)) / 2.0f;
		
		// Calculate positions.
		mPositions = {
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

		for (auto &position : mPositions)
		{
			position = glm::normalize(position);
		}

		// Set the default color.
		setSolidColor({ 1.0f, 1.0f, 1.0f });

		// Calculate indices.
		mIndices = {
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
		mPositions = {
			{ -1.0f, -1.0f, 0.0f },
			{  1.0f, -1.0f,	0.0f },
			{  1.0f, 1.0f,	0.0f },
			{ -1.0f, 1.0f,	0.0f }
		};

		// Set the default color.
		setSolidColor({ 1.0f, 1.0f, 1.0f });

		mTextureCoordinates = {
			{ 0.0f, 1.0f },
			{ 1.0f, 1.0f },
			{ 1.0f, 0.0f },
			{ 0.0f, 0.0f }
		};

		mIndices = {
			0, 1, 2, 
			2, 3, 0
		};
	}

	Circle::Circle(float tRadius, uint32_t tSubdivisions)
	{
		mPositions.push_back({ 0.0f, 0.0f, -2.0f });
		mIndices.push_back(0);

		float angleDivisor = (2.0f * M_PI) / tSubdivisions;
		for (size_t i = 0; i < tSubdivisions; ++i)
		{
			float c = cosf(angleDivisor * i) * tRadius;
			float s = sinf(angleDivisor * i) * tRadius;
			mPositions.push_back({ c, s, -2.0f });
			mIndices.push_back(i + 1);
		}

		mIndices.push_back(1);

		// Set the default color.
		setSolidColor({ 1.0f, 1.0f, 1.0f });
	}

} // namespace geo