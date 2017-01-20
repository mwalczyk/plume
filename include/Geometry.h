#pragma once

#include <vector>
#include <random>
#include <math.h>

#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/type_ptr.hpp"

namespace geo
{

	class IcoSphere
	{
	public:

		IcoSphere();

		inline const std::vector<glm::vec3>& getPositions() const { return mPositions; }
		inline const std::vector<glm::vec3>& getColors() const { return mColors; }
		inline const std::vector<glm::vec3>& getNormals() const { return mNormals; }
		inline const std::vector<glm::vec2>& getTexcoords() const { return mTexcoords; }
		inline const std::vector<uint32_t>& getIndices() const { return mIndices; }

	private:

		std::vector<glm::vec3> mPositions;
		std::vector<glm::vec3> mColors;
		std::vector<glm::vec3> mNormals;
		std::vector<glm::vec2> mTexcoords;
		std::vector<uint32_t> mIndices;
	};

} // namespace geo