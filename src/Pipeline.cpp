#include "Pipeline.h"

namespace vk
{
	static const std::string spectraUniformNames[] =
	{
		"spTime",
		"spResolution",
		"spMouse"
	};

	static const std::string spectraInputNames[] =
	{
		"spPosition",
		"spColor",
		"spNormal",
		"spTexcoord"
	};

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
		// Set up the default viewport.
		mViewport = {};
		mViewport.x = 0;
		mViewport.y = 0;
		mViewport.width = 640;
		mViewport.height = 480;
		mViewport.minDepth = 0.0f;
		mViewport.maxDepth = 1.0f;

		// Set up the default scissor region.
		mScissor = {};
		mScissor.extent = { static_cast<uint32_t>(mViewport.width), static_cast<uint32_t>(mViewport.height) };
		mScissor.offset = { 0, 0 };

		// Set up parameters for the default rasterization state.
		mPolygonMode = VK_POLYGON_MODE_FILL;

		// Set up the default input assembly.
		mPrimitiveRestart = VK_FALSE;
		mPrimitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		// Set up the default pipeline color blend attachment state (no blending).
		mPipelineColorBlendAttachmentState = {};
		mPipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		mPipelineColorBlendAttachmentState.blendEnable = VK_FALSE;	
	}
   
	VkVertexInputBindingDescription Pipeline::createVertexInputBindingDescription(uint32_t tBinding, uint32_t tStride, VkVertexInputRate tVertexInputRate)
	{
		VkVertexInputBindingDescription vertexInputBindingDescription = {};
		vertexInputBindingDescription.binding = tBinding;
		vertexInputBindingDescription.inputRate = tVertexInputRate;
		vertexInputBindingDescription.stride = tStride;

		return vertexInputBindingDescription;
	}

	VkVertexInputAttributeDescription Pipeline::createVertexInputAttributeDescription(uint32_t tBinding, VkFormat tFormat, uint32_t tLocation, uint32_t tOffset)
	{
		VkVertexInputAttributeDescription vertexInputAttributeDescription = {};
		vertexInputAttributeDescription.binding = tBinding;
		vertexInputAttributeDescription.format = tFormat;
		vertexInputAttributeDescription.location = tLocation;
		vertexInputAttributeDescription.offset = tOffset;

		return vertexInputAttributeDescription;
	}

	VkPipelineColorBlendAttachmentState Pipeline::createAlphaBlendingAttachmentState()
	{
		VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {};
		pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		pipelineColorBlendAttachmentState.blendEnable = VK_TRUE;
		pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	
		return pipelineColorBlendAttachmentState;
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
			throw std::runtime_error("At least one vertex and one fragment shader stage are required to build a graphics pipeline");
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
		rasterizationStateCreateInfo.polygonMode = tOptions.mPolygonMode;
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
	
		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.attachmentCount = 1;
		colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[3] = 0.0f;
		colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;		// If true, the logic op described here will override the blend modes for every attached framebuffer.
		colorBlendStateCreateInfo.pAttachments = &tOptions.mPipelineColorBlendAttachmentState;
		colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

		// A limited amount of the pipeline state can be changed without recreating the entire pipeline - see VkPipelineDynamicStateCreateInfo.

		buildDescriptorSetLayouts();

		// Get all of the values in the push constant ranges map. 
		std::vector<VkPushConstantRange> pushConstantRanges;
		std::transform(mPushConstantsMapping.begin(), mPushConstantsMapping.end(), std::back_inserter(pushConstantRanges), [](const auto& val) {return val.second; });

		// Get all of the values in the descriptor set layouts map.
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		std::transform(mDescriptorSetLayoutsMapping.begin(), mDescriptorSetLayoutsMapping.end(), std::back_inserter(descriptorSetLayouts), [](const auto& val) {return val.second; });

		// Encapsulate any descriptor sets and push constant ranges into a pipeline layout.
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
		pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
		pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());;
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		auto result = vkCreatePipelineLayout(mDevice->getHandle(), &pipelineLayoutCreateInfo, nullptr, &mPipelineLayoutHandle);
		assert(result == VK_SUCCESS);

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

	VkDescriptorSetLayout Pipeline::getDescriptorSetLayout(uint32_t tSet) const
	{
		auto it = mDescriptorSetLayoutsMapping.find(tSet);

		if (it == mDescriptorSetLayoutsMapping.end())
		{
			std::ostringstream os;
			os << "Descriptor set layout at set " << tSet << " not found";
			throw std::runtime_error(os.str());
		}

		return it->second;
	}

	VkDescriptorPool Pipeline::createCompatibleDescriptorPool(uint32_t tSet, uint32_t tMaxSets)
	{
		// First, make sure that a descriptor set with this index has been recorded.
		if (mDescriptorsMapping.find(tSet) == mDescriptorsMapping.end())
		{
			return VK_NULL_HANDLE;
		}

		// Create a descriptor pool size structure for each of the descriptors in this set.
		std::vector<VkDescriptorPoolSize> descriptorPoolSizes;
		for (const auto &descriptorSetLayoutBinding : mDescriptorsMapping[tSet])
		{
			VkDescriptorPoolSize descriptorPoolSize = {};
			descriptorPoolSize.descriptorCount = descriptorSetLayoutBinding.descriptorCount;
			descriptorPoolSize.type = descriptorSetLayoutBinding.descriptorType;

			descriptorPoolSizes.push_back(descriptorPoolSize);
		}

		// Finally, create the descriptor pool from the list of descriptor pool size structures above.
		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
		descriptorPoolCreateInfo.maxSets = tMaxSets;
		descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
		descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

		VkDescriptorPool descriptorPoolHandle;

		auto result = vkCreateDescriptorPool(mDevice->getHandle(), &descriptorPoolCreateInfo, nullptr, &descriptorPoolHandle);
		assert(result == VK_SUCCESS);

		return descriptorPoolHandle;
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

	void Pipeline::buildDescriptorSetLayouts()
	{
		for (const auto &mapping : mDescriptorsMapping)
		{
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
			descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(mDescriptorsMapping[mapping.first].size());
			descriptorSetLayoutCreateInfo.pBindings = mDescriptorsMapping[mapping.first].data();
			descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

			auto result = vkCreateDescriptorSetLayout(mDevice->getHandle(), &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout);
			assert(result == VK_SUCCESS);

			mDescriptorSetLayoutsMapping.insert(std::make_pair(mapping.first, descriptorSetLayout));
		}
	}

	std::ostream& operator<<(std::ostream &tStream, const PipelineRef &tPipeline)
	{
		tStream << "Pipeline object: " << tPipeline->mPipelineHandle << std::endl;

		tStream << "Push constants details:" << std::endl;
		for (const auto &mapping : tPipeline->mPushConstantsMapping)
		{
			tStream << "\tPush constant named: " << mapping.first << ":" << std::endl;
			tStream << "\t\tOffset: " << mapping.second.offset << std::endl;
			tStream << "\t\tSize: " << mapping.second.size << std::endl;
			tStream << "\t\tShader stage flags: " << shaderStageAsString(mapping.second.stageFlags) << std::endl;
		}

		tStream << "Descriptor set details:" << std::endl;
		for (const auto &mapping : tPipeline->mDescriptorsMapping)
		{
			tStream << "\tDescriptor set #" << mapping.first << ":" << std::endl;
			for (const auto &descriptorSetLayoutBinding : mapping.second)
			{
				tStream << "\t\tDescriptor at binding: " << descriptorSetLayoutBinding.binding << std::endl;
				tStream << "\t\t\tDescriptor count: " << descriptorSetLayoutBinding.descriptorCount << std::endl;
				tStream << "\t\t\tDescriptor type: " << descriptorTypeAsString(descriptorSetLayoutBinding.descriptorType) << std::endl;
				tStream << "\t\t\tShader stage flags: " << shaderStageAsString(descriptorSetLayoutBinding.stageFlags) << std::endl;
			}
		}

		return tStream;
	}
	
} // namespace vk