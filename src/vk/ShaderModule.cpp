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

#include "ShaderModule.h"

namespace graphics
{

	namespace 
	{

		const std::string plume_uniform_names[] =
		{
			"pl_time",
			"pl_resolution",
			"pl_mouse"
		};

		const std::string plume_input_names[] =
		{
			"pl_position",
			"pl_color",
			"pl_normal",
			"pl_texcoord"
		};

		uint32_t get_size_from_type(spirv_cross::SPIRType base_type, uint32_t rows, uint32_t cols)
		{
			uint32_t size = 0;
			switch (base_type.basetype)
			{
			case spirv_cross::SPIRType::Float:
				size = rows * cols * sizeof(float);
				break;
			case spirv_cross::SPIRType::Double:
				size = rows * cols * sizeof(double);
				break;
			case spirv_cross::SPIRType::Int:
				size = rows * cols * sizeof(int);
				break;
			case spirv_cross::SPIRType::Int64:
				size = rows * cols * sizeof(int64_t);
				break;
			case spirv_cross::SPIRType::UInt:
				size = rows * cols * sizeof(unsigned int);
				break;
			case spirv_cross::SPIRType::UInt64:
				size = rows * cols * sizeof(uint64_t);
				break;
			case spirv_cross::SPIRType::Boolean:
				size = rows * cols * sizeof(bool);
				break;
			case spirv_cross::SPIRType::Char:
				size = rows * cols * sizeof(char);
				break;
			case spirv_cross::SPIRType::AtomicCounter:
				break;
			case spirv_cross::SPIRType::Sampler:
				break;
			case spirv_cross::SPIRType::SampledImage:
				break;
			case spirv_cross::SPIRType::Struct:
				break;
			default:
				// Unknown type
				break;
			}

			return size;
		}

		vk::ShaderStageFlagBits spv_to_vk_execution_mode(spv::ExecutionModel mode)
		{
			switch (mode)
			{
			case spv::ExecutionModelVertex:
				return vk::ShaderStageFlagBits::eVertex;
			case spv::ExecutionModelFragment:
				return vk::ShaderStageFlagBits::eFragment;
			case spv::ExecutionModelTessellationControl:
				return vk::ShaderStageFlagBits::eTessellationControl;
			case spv::ExecutionModelTessellationEvaluation:
				return vk::ShaderStageFlagBits::eTessellationEvaluation;
			case spv::ExecutionModelGeometry:
				return vk::ShaderStageFlagBits::eGeometry;
			case spv::ExecutionModelGLCompute:
			case spv::ExecutionModelKernel:
				return vk::ShaderStageFlagBits::eCompute;
			default:
				break;
			}
		}

	} // anonymous

	ShaderModule::ShaderModule(DeviceWeakRef device, const fsys::FileResource& resource) :
		
		m_device(device)
	{
		DeviceRef device_shared = m_device.lock();

		auto shader_src = resource.contents;
		if (shader_src.size() % 4)
		{
			throw std::runtime_error("Shader source code is an invalid size");
		}

		// Store the SPIR-V code for reflection.
		auto p_code = reinterpret_cast<const uint32_t*>(shader_src.data());
		m_shader_code = std::vector<uint32_t>(p_code, p_code + shader_src.size() / sizeof(uint32_t));

		// Create the actual shader module.
		vk::ShaderModuleCreateInfo shader_module_create_info;
		shader_module_create_info.codeSize = shader_src.size();
		shader_module_create_info.pCode = p_code;
		
		m_shader_module_handle = device_shared->get_handle().createShaderModule(shader_module_create_info);

		perform_reflection();
	}

	ShaderModule::~ShaderModule()
	{
		DeviceRef device_shared = m_device.lock();

		device_shared->get_handle().destroyShaderModule(m_shader_module_handle);
	}

	void ShaderModule::perform_reflection()
	{
		// Parse the shader resources.
		spirv_cross::CompilerGLSL compiler_glsl = spirv_cross::CompilerGLSL(m_shader_code);
		spirv_cross::ShaderResources shader_resources = compiler_glsl.get_shader_resources();
		m_shader_stage = spv_to_vk_execution_mode(compiler_glsl.get_execution_model());
		m_entry_points = compiler_glsl.get_entry_points();

		// Parse push constants. Note that there can only be one push constant block,
		// so the outer for-loop below will only ever execute once.
		for (const auto &resource : shader_resources.push_constant_buffers)
		{
			auto ranges = compiler_glsl.get_active_buffer_ranges(resource.id);
			for (auto& range : ranges)
			{
				PushConstant push_constant;
				push_constant.index = range.index;
				push_constant.name = compiler_glsl.get_member_name(resource.base_type_id, range.index);
				push_constant.offset = range.offset;
				push_constant.size = range.range;

				if (push_constant.offset % 4 != 0 ||
					push_constant.size % 4 != 0)
				{
					throw std::runtime_error("Push constant ranges must have an offset and size that is divisible by 4");
				}

				m_push_constants.emplace_back(push_constant);
			}
		}

		// Parse stage inputs.
		for (const auto& resource : shader_resources.stage_inputs)
		{
			auto type = compiler_glsl.get_type(resource.type_id);
			
			StageInput input;
			input.layout_location = compiler_glsl.get_decoration(resource.id, spv::Decoration::DecorationLocation);
			input.name = resource.name;
			input.size = get_size_from_type(type, type.vecsize, 1);

			//std::cout << "Stage input - location: " << input.layout_location << ", name: " << input.name << ", size: " << input.size << " (rows: " << type.vecsize << ", cols: " << type.columns << ")\n";
			m_stage_inputs.emplace_back(input);
		}

		// Stage outputs
		for (const auto &resource : shader_resources.stage_outputs)
		{
			// TODO
		}

		// Sampled images
		for (const auto &resource : shader_resources.sampled_images)
		{
			resource_to_descriptor(compiler_glsl, resource, vk::DescriptorType::eCombinedImageSampler);
		}

		// Seperate samplers
		for (const auto &resource : shader_resources.separate_samplers)
		{
			resource_to_descriptor(compiler_glsl, resource, vk::DescriptorType::eSampler);
		}

		// Separate images
		for (const auto &resource : shader_resources.separate_images)
		{
			resource_to_descriptor(compiler_glsl, resource, vk::DescriptorType::eSampledImage);
		}

		// Atomic counters
		for (const auto &resource : shader_resources.atomic_counters)
		{
			// TODO
		}

		// Subpass inputs
		for (const auto &resource : shader_resources.subpass_inputs)
		{
			resource_to_descriptor(compiler_glsl, resource, vk::DescriptorType::eInputAttachment);
		}

		// Storage buffers (SSBOs)
		for (const auto &resource : shader_resources.storage_buffers)
		{
			resource_to_descriptor(compiler_glsl, resource, vk::DescriptorType::eStorageBuffer);
		}

		// Storage images
		for (const auto &resource : shader_resources.storage_images)
		{
			resource_to_descriptor(compiler_glsl, resource, vk::DescriptorType::eStorageImage);
		}

		// Uniform buffers (UBOs)
		for (const auto &resource : shader_resources.uniform_buffers)
		{
			resource_to_descriptor(compiler_glsl, resource, vk::DescriptorType::eUniformBuffer);
		}
	}

	void ShaderModule::resource_to_descriptor(const spirv_cross::CompilerGLSL& compiler, const spirv_cross::Resource& resource, vk::DescriptorType descriptor_type)
	{
		// TODO: parse the descriptor count (array size)
		auto full_type = compiler.get_type(resource.type_id);
	
		vk::DescriptorSetLayoutBinding descriptor_set_layout_binding;
		descriptor_set_layout_binding.binding = compiler.get_decoration(resource.id, spv::Decoration::DecorationBinding);
		descriptor_set_layout_binding.descriptorCount = 1;
		descriptor_set_layout_binding.descriptorType = descriptor_type;
		descriptor_set_layout_binding.pImmutableSamplers = nullptr;
		descriptor_set_layout_binding.stageFlags = vk::ShaderStageFlagBits::eAll; // TODO: for now, use `all` to prevent error messages - should be: m_shader_stage;
		
		Descriptor descriptor;
		descriptor.layout_set = compiler.get_decoration(resource.id, spv::Decoration::DecorationDescriptorSet);
		descriptor.name = resource.name;
		descriptor.layout_binding = descriptor_set_layout_binding;

		m_descriptors.push_back(descriptor);
	}

} // namespace graphics