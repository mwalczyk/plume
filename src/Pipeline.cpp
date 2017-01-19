#include "Pipeline.h"

namespace vk
{
	static std::string spectraUniformNames[] =
	{
		"u_time",
		"u_resolution",
		"u_mouse"
	};

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
		VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
		shaderModuleCreateInfo.codeSize = shaderSrc.size();
		shaderModuleCreateInfo.pCode = pCode;
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		auto result = vkCreateShaderModule(mDevice->getHandle(), &shaderModuleCreateInfo, nullptr, &mShaderModuleHandle);
		assert(result == VK_SUCCESS);

		performReflection();
	}

	ShaderModule::~ShaderModule()
	{
		vkDestroyShaderModule(mDevice->getHandle(), mShaderModuleHandle, nullptr);
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
			descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptor.name = resource.name;
			
			mDescriptors.push_back(descriptor);
		}
	}

	static std::string descriptorTypeAsString(VkDescriptorType tDescriptorType)
	{
		std::string output = "";
		switch (tDescriptorType)
		{
		case VK_DESCRIPTOR_TYPE_SAMPLER: output = "SAMPLER"; break;
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: output = "COMBINED IMAGE SAMPLER"; break;
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: output = "SAMPLED IMAGE"; break;
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: output = "STORAGE IMAGE"; break;
		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: output = "UNIFORM TEXEL BUFFER"; break;
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: output = "STORAGE TEXEL BUFFER"; break;
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: output = "UNIFORM BUFFER"; break;
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: output = "STORAGE BUFFER"; break;
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: output = "UNIFORM BUFFER DYNAMIC"; break;
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: output = "STORAGE BUFFER DYNAMIC"; break;
		case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: output = "INPUT ATTACHMENT"; break;
		default: output = "UNKNOWN DESCRIPTOR TYPE"; break;
		}

		return output;
	}

	static std::string shaderStageAsString(VkShaderStageFlags tShaderStageFlags)
	{
		if (tShaderStageFlags & VK_SHADER_STAGE_ALL || tShaderStageFlags & VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM)
		{
			return "ALL";
		}
		if (tShaderStageFlags & VK_SHADER_STAGE_ALL_GRAPHICS)
		{
			return "ALL GRAPHICS";
		}
		
		std::string output = "";
		if (tShaderStageFlags & VK_SHADER_STAGE_VERTEX_BIT)
		{
			output += "VERTEX | ";
		}
		if (tShaderStageFlags & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
		{
			output += "TESSELLATION CONTROL | ";
		}
		if (tShaderStageFlags & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
		{
			output += "TESSELLATION EVALUATION | ";
		}
		if (tShaderStageFlags & VK_SHADER_STAGE_GEOMETRY_BIT)
		{
			output += "GEOMETRY | ";
		}
		if (tShaderStageFlags & VK_SHADER_STAGE_COMPUTE_BIT)
		{
			output += "COMPUTE | ";
		}

		return output;
	}

	Pipeline::Options::Options()
	{
		mViewport = {};
		mViewport.x = 0;
		mViewport.y = 0;
		mViewport.width = 640;
		mViewport.height = 480;
		mViewport.minDepth = 0.0f;
		mViewport.maxDepth = 1.0f;

		mScissor = {};
		mScissor.extent = { 640, 480 };
		mScissor.offset = { 0, 0 };

		mPrimitiveRestart = VK_FALSE;
		mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}

	Pipeline::Pipeline(const DeviceRef &tDevice, const RenderPassRef &tRenderPass, const Options &tOptions) :
		mDevice(tDevice),
		mRenderPass(tRenderPass)
	{		
		// Group the create info structures together.
		std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos;

		bool foundVertexShader = false;
		bool foundFragmentShader = false;
		for (const auto &shaderStage : tOptions.mShaderStages)
		{
			if (shaderStage.second & VK_SHADER_STAGE_VERTEX_BIT) foundVertexShader = true;
			if (shaderStage.second & VK_SHADER_STAGE_FRAGMENT_BIT) foundFragmentShader = true;
			auto shaderStageInfo = buildPipelineShaderStageCreateInfo(shaderStage.first, shaderStage.second);
			pipelineShaderStageCreateInfos.push_back(shaderStageInfo);
			addPushConstantsToGlobalMap(shaderStage.first, shaderStage.second);
			addDescriptorsToGlobalMap(shaderStage.first, shaderStage.second);
		}
		if (!foundVertexShader|| !foundFragmentShader)
		{
			throw std::runtime_error("The vertex and fragment shader stages are not optional");
		}

		// Describe the format of the vertex data that will be passed to the vertex shader.
		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
		vertexInputStateCreateInfo.pVertexAttributeDescriptions = tOptions.mVertexInputAttributeDescriptions.data();
		vertexInputStateCreateInfo.pVertexBindingDescriptions = tOptions.mVertexInputBindingDescriptions.data();
		vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(tOptions.mVertexInputAttributeDescriptions.size());
		vertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(tOptions.mVertexInputBindingDescriptions.size());

		// Describe the type of geometry that will be drawn and if primitive restart should be enabled.
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
		inputAssemblyStateCreateInfo.primitiveRestartEnable = tOptions.mPrimitiveRestart;
		inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCreateInfo.topology = tOptions.mPrimitiveTopology;

		// Combine the viewport and scissor settings into a viewport state structure.
		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
		viewportStateCreateInfo.pScissors = &tOptions.mScissor;
		viewportStateCreateInfo.pViewports = &tOptions.mViewport;
		viewportStateCreateInfo.scissorCount = 1;
		viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.viewportCount = 1;

		// Configure the rasterizer.
		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
		rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;	// Turn on backface culling.
		rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
		rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
		rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
		rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
		rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizationStateCreateInfo.lineWidth = 1.0f;
		rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

		// Configure multisampling (anti-aliasing): for now, disable this feature.
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
		multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
		multisampleStateCreateInfo.minSampleShading = 1.0f;
		multisampleStateCreateInfo.pSampleMask = nullptr;
		multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

		// For now, we are not using depth and stencil tests.

		// Configure color blending, which determines how new fragments are composited with colors that are already in the framebuffer.
		// Note that there are two structures necessary for setting up color blending because each attached framebuffer has a VkPipelineColorBlendAttachmentState, 
		// while a single VkPipelineColorBlendStateCreateInfo structure contains the global color blending settings.
		VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
		colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmentState.blendEnable = VK_FALSE;		// Blending is currently disabled, so the rest of these parameters are optional.
		colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.attachmentCount = 1;
		colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[3] = 0.0f;
		colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;		// If true, the logic op described here will override the blend modes for every attached framebuffer.
		colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
		colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

		// A limited amount of the pipeline state can be changed without recreating the entire pipeline - see VkPipelineDynamicStateCreateInfo.

		// Get all of the values in the push constant ranges map. 
		std::vector<VkPushConstantRange> pushConstantRanges;
		std::transform(mPushConstantsMapping.begin(), mPushConstantsMapping.end(), std::back_inserter(pushConstantRanges), [](const auto& val) {return val.second; });

		// Generate all of the handles to the descriptor set layouts.
		mDescriptorSetLayoutHandles.resize(mDescriptorsMapping.size());
		
		for (size_t i = 0; i < mDescriptorSetLayoutHandles.size(); ++i)
		{
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
			descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(mDescriptorsMapping[i].size());
			descriptorSetLayoutCreateInfo.pBindings = mDescriptorsMapping[i].data();
			descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

			auto result = vkCreateDescriptorSetLayout(mDevice->getHandle(), &descriptorSetLayoutCreateInfo, nullptr, &mDescriptorSetLayoutHandles[i]);
			assert(result == VK_SUCCESS);
		}	

		// Encapsulate any descriptor sets and push constant ranges into a pipeline layout.
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
		pipelineLayoutCreateInfo.pSetLayouts = mDescriptorSetLayoutHandles.data();
		pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
		pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(mDescriptorSetLayoutHandles.size());;
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		auto result = vkCreatePipelineLayout(mDevice->getHandle(), &pipelineLayoutCreateInfo, nullptr, &mPipelineLayoutHandle);
		assert(result == VK_SUCCESS);

		std::cout << "Successfully created pipeline layout\n";

		// Aggregate all of the structures above to create a graphics pipeline.
		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
		graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	
		graphicsPipelineCreateInfo.basePipelineIndex = -1;
		graphicsPipelineCreateInfo.layout = mPipelineLayoutHandle;
		graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
		graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
		graphicsPipelineCreateInfo.pDynamicState = nullptr;
		graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
		graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
		graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
		graphicsPipelineCreateInfo.pStages = pipelineShaderStageCreateInfos.data();
		graphicsPipelineCreateInfo.pTessellationState = nullptr;
		graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
		graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
		graphicsPipelineCreateInfo.renderPass = mRenderPass->getHandle();
		graphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(pipelineShaderStageCreateInfos.size());
		graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineCreateInfo.subpass = 0;

		result = vkCreateGraphicsPipelines(mDevice->getHandle(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &mPipelineHandle);
		assert(result == VK_SUCCESS);

		std::cout << "Successfully created pipeline\n";
	}

	Pipeline::~Pipeline()
	{
		vkDestroyPipeline(mDevice->getHandle(), mPipelineHandle, nullptr);
	}

	VkPushConstantRange Pipeline::getPushConstantsMember(const std::string &tMemberName) const
	{
		auto it = mPushConstantsMapping.find(tMemberName);

		if (it == mPushConstantsMapping.end())
		{
			throw std::runtime_error("Push constant with name " + tMemberName + " not found");
		}

		return it->second;
	}

	VkPipelineShaderStageCreateInfo Pipeline::buildPipelineShaderStageCreateInfo(const ShaderModuleRef &tShaderModule, VkShaderStageFlagBits tShaderStageFlagBits)
	{
		VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {};
		pipelineShaderStageCreateInfo.module = tShaderModule->getHandle();
		pipelineShaderStageCreateInfo.pName = tShaderModule->getEntryPoints()[0].c_str();
		pipelineShaderStageCreateInfo.pSpecializationInfo = nullptr;
		pipelineShaderStageCreateInfo.stage = tShaderStageFlagBits;
		pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

		return pipelineShaderStageCreateInfo;
	}

	void Pipeline::addPushConstantsToGlobalMap(const ShaderModuleRef &tShaderModule, VkShaderStageFlagBits tShaderStageFlagBits)
	{
		uint32_t maxPushConstantsSize = mDevice->getPhysicalDeviceProperties().limits.maxPushConstantsSize;

		for (const auto &pushConstant : tShaderModule->getPushConstants())
		{
			VkPushConstantRange pushConstantRange = {};
			pushConstantRange.offset = pushConstant.offset;
			pushConstantRange.size = pushConstant.size;
			pushConstantRange.stageFlags = tShaderStageFlagBits;

			mPushConstantsMapping.insert({ pushConstant.name, pushConstantRange });
		}
	}

	void Pipeline::addDescriptorsToGlobalMap(const ShaderModuleRef &tShaderModule, VkShaderStageFlagBits tShaderStageFlagBits)
	{
		for (const auto &descriptor : tShaderModule->getDescriptors())
		{
			// for every descriptor found in this shader stage
			uint32_t set = descriptor.layoutSet;

			VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
			descriptorSetLayoutBinding.binding = descriptor.layoutBinding;
			descriptorSetLayoutBinding.descriptorCount = descriptor.descriptorCount;
			descriptorSetLayoutBinding.descriptorType = descriptor.descriptorType;
			descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
			descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;

			auto it = mDescriptorsMapping.find(set);
			if (it == mDescriptorsMapping.end())
			{
				std::vector<VkDescriptorSetLayoutBinding> freshDescriptorSetLayoutBindings = { descriptorSetLayoutBinding };
				mDescriptorsMapping.insert(std::make_pair(set, freshDescriptorSetLayoutBindings));
			}
			else
			{
				// Only add this entry if it doesn't already exist in this set's list of descriptors.
				auto &existingDescriptorSetLayoutBindings = (*it).second;
				auto it = std::find_if(existingDescriptorSetLayoutBindings.begin(), existingDescriptorSetLayoutBindings.end(), 
					[&](const VkDescriptorSetLayoutBinding &tDescriptorSetLayoutBinding) {
						return tDescriptorSetLayoutBinding.binding == descriptorSetLayoutBinding.binding;
					});

				if (it == existingDescriptorSetLayoutBindings.end())
				{
					existingDescriptorSetLayoutBindings.push_back(descriptorSetLayoutBinding);
				}
			}
		}			
	}

	std::ostream& operator<<(std::ostream &tStream, const PipelineRef &tPipeline)
	{
		tStream << "Pipeline object: " << tPipeline->mPipelineHandle << std::endl;

		// Print push constants
		for (const auto &mapping : tPipeline->mPushConstantsMapping)
		{
			tStream << "Push constant named: " << mapping.first << ":" << std::endl;
			tStream << "\tOffset: " << mapping.second.offset << std::endl;
			tStream << "\tSize: " << mapping.second.size << std::endl;
			tStream << "\tShader stage flags: " << shaderStageAsString(mapping.second.stageFlags) << std::endl;
		}

		// Print descriptors
		for (const auto &mapping : tPipeline->mDescriptorsMapping)
		{
			tStream << "Descriptor set #" << mapping.first << ":" << std::endl;
			for (const auto &descriptorSetLayoutBinding : mapping.second)
			{
				tStream << "\tDescriptor at binding: " << descriptorSetLayoutBinding.binding << std::endl;
				tStream << "\t\tDescriptor count: " << descriptorSetLayoutBinding.descriptorCount << std::endl;
				tStream << "\t\tDescriptor type: " << descriptorTypeAsString(descriptorSetLayoutBinding.descriptorType) << std::endl;
				tStream << "\t\tShader stage flags: " << shaderStageAsString(descriptorSetLayoutBinding.stageFlags) << std::endl;
			}
		}

		return tStream;
	}
	
} // namespace vk