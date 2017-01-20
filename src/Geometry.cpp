#include "Geometry.h"

namespace geo
{

	IcoSphere::IcoSphere()
	{
		float t = (1.0f + sqrtf(5.0f)) / 2.0f;
		
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

		for (auto &p : mPositions)
		{
			p = glm::normalize(p);
		}

		std::random_device rand;
		std::mt19937 mersenne(rand());
		std::uniform_real_distribution<float> distribution(0.0f, 1.0f);

		mColors.resize(mPositions.size());
		for (size_t i = 0; i < mColors.size(); ++i)	
		{
			mColors[i] = { distribution(mersenne), distribution(mersenne), distribution(mersenne) };
		}

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

} // namespace geo