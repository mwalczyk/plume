#include "Geometry.h"

namespace geo
{

	IcoSphere::IcoSphere()
	{
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

	IcoSphere& IcoSphere::setRandomColors()
	{
		std::random_device rand;
		std::mt19937 mersenne(rand());
		std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

		mColors.clear();
		mColors.resize(getVertexCount());

		std::generate(mColors.begin(), mColors.end(), [&]() -> glm::vec3 { return{ distribution(mersenne), distribution(mersenne), distribution(mersenne) }; });

		return *this;
	}

	IcoSphere& IcoSphere::setColorFrom(VertexAttribute tAttribute, const std::function<glm::vec3(glm::vec3)> tFunc)
	{
		mColors.clear();
		mColors.resize(getVertexCount());

		const auto *inputData = getVertexAttribute(tAttribute);
		size_t inputAttributeDimensions = getVertexAttributeDimensions(tAttribute);

		if (inputAttributeDimensions == 3)
		{
			//fillColors(reinterpret_cast<const glm::vec3*>(inputData), tFunc);
		}
		else if (inputAttributeDimensions == 2)
		{
			//fillColors(reinterpret_cast<const glm::vec2*>(inputData), tFunc);
		}

		for (size_t i = 0; i < mColors.size(); i++)
		{
			//1111mColors[i] = tFunc(inputData[i]);
		}

		return *this;
	}

	IcoSphere& IcoSphere::setColorFrom(VertexAttribute tAttribute, const std::function<glm::vec3(glm::vec2)> tFunc)
	{
		mColors.clear();
		mColors.resize(getVertexCount());

		return *this;
	}

	float* IcoSphere::getVertexAttribute(VertexAttribute tAttribute) 
	{
		switch (tAttribute)
		{
		case geo::VertexAttribute::ATTRIBUTE_POSITION: return reinterpret_cast<float*>(mPositions.data());
		case geo::VertexAttribute::ATTRIBUTE_COLOR: return reinterpret_cast<float*>(mColors.data());
		case geo::VertexAttribute::ATTRIBUTE_NORMAL: return reinterpret_cast<float*>(mNormals.data());
		case geo::VertexAttribute::ATTRIBUTE_TEXTURE_COORDINATES: return reinterpret_cast<float*>(mTextureCoordinates.data());
		}
	}

	size_t IcoSphere::getVertexAttributeDimensions(VertexAttribute tAttribute)
	{
		switch (tAttribute)
		{
		case geo::VertexAttribute::ATTRIBUTE_POSITION: 
		case geo::VertexAttribute::ATTRIBUTE_COLOR: 
		case geo::VertexAttribute::ATTRIBUTE_NORMAL: return 3;
		case geo::VertexAttribute::ATTRIBUTE_TEXTURE_COORDINATES: return 2;
		}
	}

	void IcoSphere::fillColors(const glm::vec2 *tInputData, const std::function<glm::vec3(glm::vec2)> tFunc)
	{

	}

	void IcoSphere::fillColors(const glm::vec3 *tInputData, const std::function<glm::vec3(glm::vec3)> tFunc)
	{

	}

} // namespace geo