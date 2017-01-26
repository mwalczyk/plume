#pragma once

#include <memory>

#include "spirv_glsl.hpp"

#include "Platform.h"
#include "Device.h"
#include "ResourceManager.h"

namespace graphics
{

	class ShaderModule;
	using ShaderModuleRef = std::shared_ptr<ShaderModule>;

	class ShaderModule
	{

	public:

		//! A struct representing a memmber within a push constants block inside of a GLSL shader. For example:
		//! layout (std430, push_constant) uniform PushConstants
		//! {
		//!		float time;		<--- this
		//! } constants;
		struct PushConstant
		{
			uint32_t index;
			uint32_t size;
			uint32_t offset;
			std::string name;
		};

		//! A struct representing an input to a shader stage. For example:
		//! layout (location = 0) in vec3 inPosition;
		struct StageInput
		{
			uint32_t layoutLocation;
			uint32_t size;
			std::string name;
		};

		//! A struct representing a descriptor inside of a GLSL shader. For example:
		//! layout (set = 0, binding = 1) uniform UniformBufferObject	<--- this
		//! {
		//!		mat4 model;
		//!		mat4 view;
		//!		mat4 projection
		//! } ubo;
		struct Descriptor
		{
			uint32_t layoutSet;
			uint32_t layoutBinding;
			uint32_t descriptorCount;
			vk::DescriptorType descriptorType;
			std::string name;
		};

		//! Factory method for returning a new ShaderModuleRef.
		static ShaderModuleRef create(const DeviceRef &tDevice, const FileResource &tResource)
		{
			return std::make_shared<ShaderModule>(tDevice, tResource);
		}

		ShaderModule(const DeviceRef &tDevice, const FileResource &tResource);
		~ShaderModule();

		inline vk::ShaderModule getHandle() const { return mShaderModuleHandle; }

		//! Retrieve the binary SPIR-V shader code that is held by this shader.
		inline const std::vector<uint32_t>& getShaderCode() const { return mShaderCode; }

		//! Retrieve a list of available entry points within this GLSL shader (usually "main").
		inline const std::vector<std::string>& getEntryPoints() const { return mEntryPoints; }

		//! Retrieve a list of low-level details about the push constants contained within this GLSL shader.
		inline const std::vector<PushConstant> getPushConstants() const { return mPushConstants; }

		//! Retrieve a list of low-level details about the descriptors contained within this GLSL shader.
		inline const std::vector<Descriptor>& getDescriptors() const { return mDescriptors; }

	private:

		void performReflection();

		//! Used during reflection to convert a shader resource into a descriptor struct.
		void resourceToDescriptor(const spirv_cross::CompilerGLSL &tCompiler, const spirv_cross::Resource &tResource, vk::DescriptorType tDescriptorType);

		DeviceRef mDevice;
		vk::ShaderModule mShaderModuleHandle;
		std::vector<uint32_t> mShaderCode;
		std::vector<std::string> mEntryPoints;
		std::vector<StageInput> mStageInputs;
		std::vector<PushConstant> mPushConstants;
		std::vector<Descriptor> mDescriptors;
	};

} // namespace graphics