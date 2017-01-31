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

	static uint32_t getSizeFromType(spirv_cross::SPIRType tBaseType, uint32_t tRows, uint32_t tColumns)
	{
		uint32_t size = 0;
		switch (tBaseType.basetype)
		{
		case spirv_cross::SPIRType::Float:
			size = tRows * tColumns * sizeof(float);
			break;
		case spirv_cross::SPIRType::Double:
			break;
		case spirv_cross::SPIRType::Int:
			size = tRows * tColumns * sizeof(int);
			break;
		case spirv_cross::SPIRType::Int64:
			size = tRows * tColumns * sizeof(uint64_t);
			break;
		case spirv_cross::SPIRType::UInt:
			break;
		case spirv_cross::SPIRType::UInt64:
			break;
		case spirv_cross::SPIRType::Boolean:
			break;
		case spirv_cross::SPIRType::Char:
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

	ShaderModule::ShaderModule(const DeviceRef &tDevice, const FileResource &tResource) :
		mDevice(tDevice)
	{
		auto shaderSrc = tResource.contents;
		if (shaderSrc.size() % 4)
		{
			throw std::runtime_error("Shader source code is an invalid size");
		}

		// Store the SPIR-V code for reflection.
		auto pCode = reinterpret_cast<const uint32_t*>(shaderSrc.data());
		mShaderCode = std::vector<uint32_t>(pCode, pCode + shaderSrc.size() / sizeof(uint32_t));

		// Create the actual shader module.
		vk::ShaderModuleCreateInfo shaderModuleCreateInfo;
		shaderModuleCreateInfo.codeSize = shaderSrc.size();
		shaderModuleCreateInfo.pCode = pCode;
		
		mShaderModuleHandle = mDevice->getHandle().createShaderModule(shaderModuleCreateInfo);

		performReflection();
	}

	ShaderModule::~ShaderModule()
	{
		mDevice->getHandle().destroyShaderModule(mShaderModuleHandle);
	}

	void ShaderModule::performReflection()
	{
		// Parse the shader resources.
		spirv_cross::CompilerGLSL compilerGlsl = spirv_cross::CompilerGLSL(mShaderCode);
		spirv_cross::ShaderResources shaderResources = compilerGlsl.get_shader_resources();

		mEntryPoints = compilerGlsl.get_entry_points();

		// Get all of the push constants (currently, Vulkan only supports one block).
		for (const auto &resource : shaderResources.push_constant_buffers)
		{
			auto ranges = compilerGlsl.get_active_buffer_ranges(resource.id);
			for (auto &range : ranges)
			{
				PushConstant pushConstant;
				pushConstant.index = range.index;
				pushConstant.name = compilerGlsl.get_member_name(resource.base_type_id, range.index);
				pushConstant.offset = range.offset;
				pushConstant.size = range.range;

				mPushConstants.emplace_back(pushConstant);
			}
		}

		// Stage inputs
		for (const auto &resource : shaderResources.stage_inputs)
		{
			auto type = compilerGlsl.get_type(resource.type_id);

			StageInput stageInput;
			stageInput.layoutLocation = compilerGlsl.get_decoration(resource.id, spv::Decoration::DecorationLocation);
			stageInput.name = resource.name;
			stageInput.size = getSizeFromType(type, type.vecsize, 1);

			mStageInputs.emplace_back(stageInput);
		}

		// Stage outputs
		for (const auto &resource : shaderResources.stage_outputs)
		{
		}

		// Sampled images
		for (const auto &resource : shaderResources.sampled_images)
		{
			resourceToDescriptor(compilerGlsl, resource, vk::DescriptorType::eCombinedImageSampler);
		}

		// Seperate samplers
		for (const auto &resource : shaderResources.separate_samplers)
		{
			resourceToDescriptor(compilerGlsl, resource, vk::DescriptorType::eSampler);
		}

		// Separate images
		for (const auto &resource : shaderResources.separate_images)
		{
			resourceToDescriptor(compilerGlsl, resource, vk::DescriptorType::eSampledImage);
		}

		// Atomic counters
		for (const auto &resource : shaderResources.atomic_counters)
		{
		}

		// Subpass inputs
		for (const auto &resource : shaderResources.subpass_inputs)
		{
		}

		// Storage buffers (SSBOs)
		for (const auto &resource : shaderResources.storage_buffers)
		{
			resourceToDescriptor(compilerGlsl, resource, vk::DescriptorType::eStorageBuffer);
		}

		// Storage images
		for (const auto &resource : shaderResources.storage_images)
		{
			resourceToDescriptor(compilerGlsl, resource, vk::DescriptorType::eStorageImage);
		}

		// Uniform buffers (UBOs)
		for (const auto &resource : shaderResources.uniform_buffers)
		{
			resourceToDescriptor(compilerGlsl, resource, vk::DescriptorType::eUniformBuffer);
		}
	}

	void ShaderModule::resourceToDescriptor(const spirv_cross::CompilerGLSL &tCompiler, const spirv_cross::Resource &tResource, vk::DescriptorType tDescriptorType)
	{
		Descriptor descriptor;
		descriptor.layoutSet = tCompiler.get_decoration(tResource.id, spv::Decoration::DecorationDescriptorSet);
		descriptor.layoutBinding = tCompiler.get_decoration(tResource.id, spv::Decoration::DecorationBinding);
		descriptor.descriptorCount = 1;
		descriptor.descriptorType = tDescriptorType;
		descriptor.name = tResource.name;

		mDescriptors.push_back(descriptor);
	}

} // namespace graphics