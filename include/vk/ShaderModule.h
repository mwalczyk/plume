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

#include "spirv_glsl.hpp"

#include "Device.h"
#include "ResourceManager.h"

namespace graphics
{

	//! Shader modules contain shader code and one or more entry points. The shader code defining a shader
	//! module must be in the SPIR-V format. Data is passed into and out of shaders using variables with 
	//! input or output storage class, respectively. User-defined inputs and outputs are connected between
	//! stages by matching their 'location' decorations.
	class ShaderModule
	{

	public:

		//! A struct representing a memmber within a push constants block inside of a GLSL shader. For example:
		//!
		//!					layout (std430, push_constant) uniform push_constants
		//!					{
		//!						float time;		
		//!							  ^^^^
		//!					} constants;
		//!
		//! Note that there can only be one push constants block, but it can be shared across multiple shader
		//! stages.
		struct PushConstant
		{
			uint32_t index;
			uint32_t size;
			uint32_t offset;
			std::string name;
		};

		//! A struct representing an input to a shader stage. For example:
		//!
		//!					layout (location = 0) in vec3 in_position;
		struct StageInput
		{
			uint32_t layout_location;
			uint32_t size;
			std::string name;
		};

		//! A struct representing a descriptor inside of a GLSL shader. For example:
		//!
		//!					layout (set = 0, binding = 1) uniform uniform_buffer_object	
		//!														  ^^^^^^^^^^^^^^^^^^^^^
		//!					{
		//!						mat4 model;
		//!						mat4 view;
		//!						mat4 projection
		//!					} ubo;
		struct Descriptor
		{
			uint32_t layout_set;
			std::string name;
			vk::DescriptorSetLayoutBinding layout_binding;
		};

		//! Factory method for constructing a new shared ShaderModule.
		static std::shared_ptr<ShaderModule> create(const Device& device, const fsys::FileResource& resouce)
		{
			return std::shared_ptr<ShaderModule>(new ShaderModule(device, resouce));
		}

		vk::ShaderModule get_handle() const { return m_shader_module_handle.get(); }

		//! Retrieve the binary SPIR-V shader code that is held by this shader.
		const std::vector<uint32_t>& get_shader_code() const { return m_shader_code; }

		//! Retrieve a list of available entry points within this GLSL shader (usually "main").
		const std::vector<std::string>& get_entry_points() const { return m_entry_points; }

		//! Retrieve a list of low-level details about the push constants contained within this GLSL shader.
		const std::vector<PushConstant>& get_push_constants() const { return m_push_constants; }

		//! Retrieve a list of low-level details about the descriptors contained within this GLSL shader.
		const std::vector<Descriptor>& get_descriptors() const { return m_descriptors; }

		//! Returns the shader stage corresponding to this module (i.e. vk::ShaderStageFlagBits::eVertex).
		vk::ShaderStageFlagBits get_stage() const { return m_shader_stage; }

	private:

		ShaderModule(const Device& device, const fsys::FileResource& resouce);

		void perform_reflection();

		//! Used during reflection to convert a shader resource into a descriptor struct.
		void resource_to_descriptor(const spirv_cross::CompilerGLSL& compiler, const spirv_cross::Resource& resource, vk::DescriptorType descriptor_type);

		const Device* m_device_ptr;
		vk::UniqueShaderModule m_shader_module_handle;

		std::vector<uint32_t> m_shader_code;
		std::vector<std::string> m_entry_points;
		std::vector<StageInput> m_stage_inputs;
		std::vector<PushConstant> m_push_constants;
		std::vector<Descriptor> m_descriptors;
		vk::ShaderStageFlagBits m_shader_stage;
	};

} // namespace graphics