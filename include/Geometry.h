#pragma once

#include <vector>
#include <algorithm>
#include <functional>
#include <random>
#include <math.h>

#include "glm/glm/glm.hpp"
#include "glm/glm/gtc/type_ptr.hpp"

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

		virtual VertexAttributeSet getActiveVertexAttributes() const = 0;
		virtual size_t getVertexCount() const = 0;

	protected:

		VertexAttributeSet mActiveAttributes;
	};

	class IcoSphere: public Geometry
	{
	public:

		IcoSphere();
		inline IcoSphere& setColors(const std::vector<glm::vec3> &tColors) { mColors = tColors; return *this; }
		inline IcoSphere& setSolidColor(const glm::vec3 &tColor) { mColors = std::vector<glm::vec3>(getVertexCount(), tColor); return *this; }
		IcoSphere& setRandomColors();
		IcoSphere& setColorFrom(VertexAttribute tAttribute, const std::function<glm::vec3(glm::vec3)> tFunc);
		IcoSphere& setColorFrom(VertexAttribute tAttribute, const std::function<glm::vec3(glm::vec2)> tFunc);

		inline VertexAttributeSet getActiveVertexAttributes() const override
		{ 
			return {VertexAttribute::ATTRIBUTE_POSITION, 
					VertexAttribute::ATTRIBUTE_COLOR, 
					VertexAttribute::ATTRIBUTE_NORMAL, 
					VertexAttribute::ATTRIBUTE_TEXTURE_COORDINATES}; 
		}

		inline size_t getVertexCount() const override { return mPositions.size(); }
		inline const std::vector<glm::vec3>& getPositions() const { return mPositions; }
		inline const std::vector<glm::vec3>& getColors() const { return mColors; }
		inline const std::vector<glm::vec3>& getNormals() const { return mNormals; }
		inline const std::vector<glm::vec2>& getTextureCoordinates() const { return mTextureCoordinates; }
		inline const std::vector<uint32_t>& getIndices() const { return mIndices; }

	private:

		float* getVertexAttribute(VertexAttribute tAttribute);
		size_t getVertexAttributeDimensions(VertexAttribute tAttribute);
		void fillColors(const glm::vec2 *tInputData, const std::function<glm::vec3(glm::vec2)> tFunc);
		void fillColors(const glm::vec3 *tInputData, const std::function<glm::vec3(glm::vec3)> tFunc);

		std::vector<glm::vec3> mPositions;
		std::vector<glm::vec3> mColors;
		std::vector<glm::vec3> mNormals;
		std::vector<glm::vec2> mTextureCoordinates;
		std::vector<uint32_t> mIndices;

		bool mHasColors;
		bool mHasNormals;
		bool mHasTextureCoordinates;
		bool mHasIndices;
	};

} // namespace geo