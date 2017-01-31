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

#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/type_ptr.hpp"

#include "Platform.h"

namespace geo
{

	enum class VertexAttribute
	{
		ATTRIBUTE_POSITION,
		ATTRIBUTE_COLOR,
		ATTRIBUTE_NORMAL,
		ATTRIBUTE_TEXTURE_COORDINATES
	};

	using VertexAttributeSet = std::vector<VertexAttribute>;

	class Geometry
	{
	public:

		virtual ~Geometry() = default;

		virtual vk::PrimitiveTopology getTopology() const = 0;
		virtual size_t getVertexAttributeDimensions(VertexAttribute tAttribute) const;

		inline size_t getVertexCount() const { return mPositions.size(); }

		inline const std::vector<glm::vec3>& getPositions() const { return mPositions; }
		inline const std::vector<glm::vec3>& getColors() const { return mColors; }
		inline const std::vector<glm::vec3>& getNormals() const { return mNormals; }
		inline const std::vector<glm::vec2>& getTextureCoordinates() const { return mTextureCoordinates; }
		inline const std::vector<uint32_t>& getIndices() const { return mIndices; }
		inline VertexAttributeSet getActiveVertexAttributes() const { return mAttributeSet; }

		float* getVertexAttribute(VertexAttribute tAttribute);


		inline void setColors(const std::vector<glm::vec3> &tColors) { mColors = tColors; }
		inline void setSolidColor(const glm::vec3 &tColor) { mColors = std::vector<glm::vec3>(getVertexCount(), tColor); }
		void setRandomColors();

	protected:

		std::vector<glm::vec3> mPositions;
		std::vector<glm::vec3> mColors;
		std::vector<glm::vec3> mNormals;
		std::vector<glm::vec2> mTextureCoordinates;
		std::vector<uint32_t> mIndices;

		VertexAttributeSet mAttributeSet;
	};

	class IcoSphere: public Geometry
	{
	public:

		IcoSphere();

		inline vk::PrimitiveTopology getTopology() const override { return vk::PrimitiveTopology::eTriangleList; }

	private:

	};

	class Grid : public Geometry
	{
	public:

		Grid();

		inline vk::PrimitiveTopology getTopology() const override { return vk::PrimitiveTopology::eTriangleStrip; }

	};

	class Circle : public Geometry
	{
	public:

		Circle() : Circle(1.0f) {};
		Circle(float tRadius, uint32_t tSubdivisions = 30);

		inline vk::PrimitiveTopology getTopology() const override { return vk::PrimitiveTopology::eTriangleFan; }

	};

} // namespace geo