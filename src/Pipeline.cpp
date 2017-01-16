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

		std::cout << "Successfully read " << fileSize << " bytes from file: " << tFileName << "\n";

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

		std::cout << "Successfully created shader module\n";

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

		// Get all of the push constants blocks and their members (currently, Vulkan only supports one block).
		for (const auto &resource : shaderResources.push_constant_buffers)
		{
			PushConstantsBlock pushConstantsBlock;
			pushConstantsBlock.layoutLocation = compilerGlsl.get_decoration(resource.id, spv::Decoration::DecorationLocation);
			pushConstantsBlock.totalSize = static_cast<uint32_t>(compilerGlsl.get_declared_struct_size(compilerGlsl.get_type(resource.base_type_id)));
			pushConstantsBlock.name = resource.name;

			std::vector<PushConstantsMember> pushConstantsMembers;

			// Store some information about each member of the push constants block.
			auto ranges = compilerGlsl.get_active_buffer_ranges(resource.id);
			for (auto &range : ranges)
			{
				auto index = range.index;
				
				// Determine the total size of this block member. 
				auto type = compilerGlsl.get_type(compilerGlsl.get_type(resource.base_type_id).member_types[range.index]);
				auto rows = type.vecsize;
				auto columns = type.columns;
				auto size = getSizeFromType(type, rows, columns);

				auto offset = static_cast<uint32_t>(range.offset);
				auto name = compilerGlsl.get_member_name(resource.base_type_id, range.index);
			
				pushConstantsMembers.emplace_back(PushConstantsMember{ index, size, offset, name });
			}

			mPushConstantsBlocksMapping.emplace(pushConstantsBlock, pushConstantsMembers);
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
		}

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
		if (!tOptions.mVertexShader || !tOptions.mFragmentShader)
		{
			throw std::runtime_error("The vertex and fragment shader stages are not optional");
		}
		
		// Group the create info structures together.
		std::vector<VkPipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos;

		if (tOptions.mVertexShader)
		{
			auto vertexShaderStageInfo = buildPipelineShaderStageCreateInfo(tOptions.mVertexShader, VK_SHADER_STAGE_VERTEX_BIT);
			pipelineShaderStageCreateInfos.push_back(vertexShaderStageInfo);
			addPushConstantRangesToGlobalMap(tOptions.mVertexShader, VK_SHADER_STAGE_VERTEX_BIT);
		}
		if (tOptions.mTessellationControlShader)
		{
			auto tessellationControlShaderStageInfo = buildPipelineShaderStageCreateInfo(tOptions.mTessellationControlShader, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
			pipelineShaderStageCreateInfos.push_back(tessellationControlShaderStageInfo);
			addPushConstantRangesToGlobalMap(tOptions.mTessellationControlShader, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
		}
		if (tOptions.mTessellationEvaluationShader)
		{
			auto tessellationEvaluationShaderStageInfo = buildPipelineShaderStageCreateInfo(tOptions.mTessellationEvaluationShader, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
			pipelineShaderStageCreateInfos.push_back(tessellationEvaluationShaderStageInfo);
			addPushConstantRangesToGlobalMap(tOptions.mTessellationEvaluationShader, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
		}
		if (tOptions.mGeometryShader)
		{
			auto geometryShaderStageInfo = buildPipelineShaderStageCreateInfo(tOptions.mGeometryShader, VK_SHADER_STAGE_GEOMETRY_BIT);
			pipelineShaderStageCreateInfos.push_back(geometryShaderStageInfo);
			addPushConstantRangesToGlobalMap(tOptions.mGeometryShader, VK_SHADER_STAGE_GEOMETRY_BIT);
		}
		if (tOptions.mFragmentShader)
		{
			auto fragmentShaderStageInfo = buildPipelineShaderStageCreateInfo(tOptions.mFragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT);
			pipelineShaderStageCreateInfos.push_back(fragmentShaderStageInfo);
			addPushConstantRangesToGlobalMap(tOptions.mFragmentShader, VK_SHADER_STAGE_FRAGMENT_BIT);
		}
		std::cout << "Creating pipeline with " << pipelineShaderStageCreateInfos.size() << " shader stages\n";

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

		// Get all of the values in the push constant ranges map. These correspond to all of the VkPushConstantRange structures 
		// found through the SPIR-V reflection process.
		std::vector<VkPushConstantRange> pushConstantRanges;
		std::transform(mPushConstantRangesMapping.begin(), mPushConstantRangesMapping.end(), std::back_inserter(pushConstantRanges), [](const auto& val) {return val.second; });

		// Encapsulate any descriptor sets and push constant ranges into a pipeline layout.
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
		pipelineLayoutCreateInfo.pSetLayouts = nullptr;
		pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
		pipelineLayoutCreateInfo.setLayoutCount = 0;
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
		auto it = mPushConstantRangesMapping.find(tMemberName);

		if (it == mPushConstantRangesMapping.end())
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

	void Pipeline::addPushConstantRangesToGlobalMap(const ShaderModuleRef &tShaderModule, VkShaderStageFlagBits tShaderStageFlagBits)
	{
		uint32_t maxPushConstantsSize = mDevice->getPhysicalDeviceProperties().limits.maxPushConstantsSize;

		for (const auto &mapping : tShaderModule->getPushConstantsBlocksMapping())
		{
			// For now, we ignore information about the block.

			if (mapping.first.totalSize > maxPushConstantsSize)
			{
				throw std::runtime_error("Push constants block exceeds the maximum size that is supported by the current physical device");
			}

			for (const auto &member : mapping.second)
			{
				VkPushConstantRange pushConstantRange = {};
				pushConstantRange.offset = member.offset;
				pushConstantRange.size = member.size;
				pushConstantRange.stageFlags = tShaderStageFlagBits;

				mPushConstantRangesMapping.insert({ member.name, pushConstantRange });
			}
		}
	}

} // namespace vk