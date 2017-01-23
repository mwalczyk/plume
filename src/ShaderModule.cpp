#include "ShaderModule.h"

namespace vksp
{
	static std::vector<uint8_t> readFile(const std::string &tFileName)
	{
		// Start reading at the end of the file to determine file size.
		std::ifstream file(tFileName, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file " + tFileName);
		}

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<uint8_t> fileContents(fileSize);

		// Go back to the beginning of the file.
		file.seekg(0);

		// Read and close the file.
		auto data = reinterpret_cast<char*>(fileContents.data());
		file.read(data, fileSize);
		file.close();

		return fileContents;
	}

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

	ShaderModule::ShaderModule(const DeviceRef &tDevice, const std::string &tFilePath) :
		mDevice(tDevice)
	{
		auto shaderSrc = readFile(tFilePath);
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
		}

		// Seperate samplers
		for (const auto &resource : shaderResources.separate_samplers)
		{
		}

		// Separate images
		for (const auto &resource : shaderResources.separate_images)
		{
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
		}

		// Storage images
		for (const auto &resource : shaderResources.storage_images)
		{
		}

		// Uniform buffers (UBOs)
		for (const auto &resource : shaderResources.uniform_buffers)
		{
			Descriptor descriptor;
			descriptor.layoutSet = compilerGlsl.get_decoration(resource.id, spv::Decoration::DecorationDescriptorSet);
			descriptor.layoutBinding = compilerGlsl.get_decoration(resource.id, spv::Decoration::DecorationBinding);
			descriptor.descriptorCount = 1;
			descriptor.descriptorType = vk::DescriptorType::eUniformBuffer;
			descriptor.name = resource.name;

			mDescriptors.push_back(descriptor);
		}
	}

} // namespace vksp