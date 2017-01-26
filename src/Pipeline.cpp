#include "Pipeline.h"

namespace graphics
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

	static std::string descriptorTypeAsString(vk::DescriptorType tDescriptorType)
	{
		std::string output = "";
		switch (tDescriptorType)
		{
		case vk::DescriptorType::eSampler: output = "SAMPLER"; break;
		case vk::DescriptorType::eCombinedImageSampler: output = "COMBINED IMAGE SAMPLER"; break;
		case vk::DescriptorType::eSampledImage: output = "SAMPLED IMAGE"; break;
		case vk::DescriptorType::eStorageImage: output = "STORAGE IMAGE"; break;
		case vk::DescriptorType::eUniformTexelBuffer: output = "UNIFORM TEXEL BUFFER"; break;
		case vk::DescriptorType::eStorageTexelBuffer: output = "STORAGE TEXEL BUFFER"; break;
		case vk::DescriptorType::eUniformBuffer: output = "UNIFORM BUFFER"; break;
		case vk::DescriptorType::eStorageBuffer: output = "STORAGE BUFFER"; break;
		case vk::DescriptorType::eUniformBufferDynamic: output = "UNIFORM BUFFER DYNAMIC"; break;
		case vk::DescriptorType::eStorageBufferDynamic: output = "STORAGE BUFFER DYNAMIC"; break;
		case vk::DescriptorType::eInputAttachment: output = "INPUT ATTACHMENT"; break;
		default: output = "UNKNOWN DESCRIPTOR TYPE"; break;
		}

		return output;
	}

	static std::string shaderStageAsString(vk::ShaderStageFlags tShaderStageFlags)
	{
		if (tShaderStageFlags == vk::ShaderStageFlagBits::eAll)
		{
			return "ALL";
		}
		if (tShaderStageFlags == vk::ShaderStageFlagBits::eAllGraphics)
		{
			return "ALL GRAPHICS";
		}
		
		std::vector<std::string> matches;
		if (tShaderStageFlags & vk::ShaderStageFlagBits::eVertex)
		{
			matches.push_back("VERTEX");
		}
		if (tShaderStageFlags & vk::ShaderStageFlagBits::eTessellationControl)
		{
			matches.push_back("TESSELLATION CONTROL");
		}
		if (tShaderStageFlags & vk::ShaderStageFlagBits::eTessellationEvaluation)
		{
			matches.push_back("TESSELLATION EVALUATION");
		}
		if (tShaderStageFlags & vk::ShaderStageFlagBits::eGeometry)
		{
			matches.push_back("GEOMETRY");
		}
		if (tShaderStageFlags & vk::ShaderStageFlagBits::eFragment)
		{
			matches.push_back("FRAGMENT");
		}
		if (tShaderStageFlags & vk::ShaderStageFlagBits::eCompute)
		{
			matches.push_back("COMPUTE");
		}
			
		std::ostringstream os;
		std::copy(matches.begin(), matches.end() - 1, std::ostream_iterator<std::string>(os, " | "));
		os << *matches.rbegin();
		return os.str();
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
		mCullModeFlags = vk::CullModeFlagBits::eBack;
		mPolygonMode = vk::PolygonMode::eFill;
		mLineWidth = 1.0f;

		// Set up the default input assembly.
		mPrimitiveRestart = VK_FALSE;
		mPrimitiveTopology = vk::PrimitiveTopology::eTriangleList; 

		// Set up the default pipeline color blend attachment state (no blending).
		mPipelineColorBlendAttachmentState = {};
		mPipelineColorBlendAttachmentState.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		mPipelineColorBlendAttachmentState.blendEnable = VK_FALSE;	
	}
   
	vk::VertexInputBindingDescription Pipeline::createVertexInputBindingDescription(uint32_t tBinding, uint32_t tStride, vk::VertexInputRate tVertexInputRate)
	{
		vk::VertexInputBindingDescription vertexInputBindingDescription;
		vertexInputBindingDescription.binding = tBinding;
		vertexInputBindingDescription.inputRate = tVertexInputRate;
		vertexInputBindingDescription.stride = tStride;

		return vertexInputBindingDescription;
	}

	vk::VertexInputAttributeDescription Pipeline::createVertexInputAttributeDescription(uint32_t tBinding, vk::Format tFormat, uint32_t tLocation, uint32_t tOffset)
	{
		vk::VertexInputAttributeDescription vertexInputAttributeDescription;
		vertexInputAttributeDescription.binding = tBinding;
		vertexInputAttributeDescription.format = tFormat;
		vertexInputAttributeDescription.location = tLocation;
		vertexInputAttributeDescription.offset = tOffset;

		return vertexInputAttributeDescription;
	}

	vk::PipelineColorBlendAttachmentState Pipeline::createAlphaBlendingAttachmentState()
	{
		vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState;
		pipelineColorBlendAttachmentState.alphaBlendOp = vk::BlendOp::eAdd;
		pipelineColorBlendAttachmentState.blendEnable = VK_TRUE;
		pipelineColorBlendAttachmentState.colorBlendOp = vk::BlendOp::eAdd;
		pipelineColorBlendAttachmentState.dstAlphaBlendFactor = vk::BlendFactor::eZero; 
		pipelineColorBlendAttachmentState.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;  
		pipelineColorBlendAttachmentState.srcAlphaBlendFactor = vk::BlendFactor::eOne; 
		pipelineColorBlendAttachmentState.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	
		return pipelineColorBlendAttachmentState;
	}

	Pipeline::Pipeline(const DeviceRef &tDevice, const RenderPassRef &tRenderPass, const Options &tOptions) :
		mDevice(tDevice),
		mRenderPass(tRenderPass)
	{		
		// Group the create info structures together.
		std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStageCreateInfos;

		bool foundVertexShader = false;
		bool foundFragmentShader = false;
		for (const auto &shaderStage : tOptions.mShaderStages)
		{
			if (shaderStage.second == vk::ShaderStageFlagBits::eVertex) foundVertexShader = true;
			if (shaderStage.second == vk::ShaderStageFlagBits::eFragment) foundFragmentShader = true;
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
		vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo;
		vertexInputStateCreateInfo.pVertexAttributeDescriptions = tOptions.mVertexInputAttributeDescriptions.data();
		vertexInputStateCreateInfo.pVertexBindingDescriptions = tOptions.mVertexInputBindingDescriptions.data();
		vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(tOptions.mVertexInputAttributeDescriptions.size());
		vertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(tOptions.mVertexInputBindingDescriptions.size());

		// Describe the type of geometry that will be drawn and if primitive restart should be enabled.
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo;
		inputAssemblyStateCreateInfo.primitiveRestartEnable = tOptions.mPrimitiveRestart;
		inputAssemblyStateCreateInfo.topology = tOptions.mPrimitiveTopology;

		// Combine the viewport and scissor settings into a viewport state structure.
		vk::PipelineViewportStateCreateInfo viewportStateCreateInfo;
		viewportStateCreateInfo.pScissors = &tOptions.mScissor;
		viewportStateCreateInfo.pViewports = &tOptions.mViewport;
		viewportStateCreateInfo.scissorCount = 1;
		viewportStateCreateInfo.viewportCount = 1;

		// Configure the rasterizer.
		vk::PipelineRasterizationStateCreateInfo rasterizationStateCreateInfo;
		rasterizationStateCreateInfo.cullMode = tOptions.mCullModeFlags;
		rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
		rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
		rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
		rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
		rasterizationStateCreateInfo.frontFace = vk::FrontFace::eClockwise;
		rasterizationStateCreateInfo.lineWidth = tOptions.mLineWidth;
		rasterizationStateCreateInfo.polygonMode = tOptions.mPolygonMode;
		rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;

		// Configure multisampling (anti-aliasing): for now, disable this feature.
		vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo;
		multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
		multisampleStateCreateInfo.minSampleShading = 1.0f;
		multisampleStateCreateInfo.pSampleMask = nullptr;
		multisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;
		multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;

		// For now, we are not using depth and stencil tests.
	
		vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo;
		colorBlendStateCreateInfo.attachmentCount = 1;
		colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[3] = 0.0f;
		colorBlendStateCreateInfo.logicOp = vk::LogicOp::eCopy;
		colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;		// If true, the logic op described here will override the blend modes for every attached framebuffer.
		colorBlendStateCreateInfo.pAttachments = &tOptions.mPipelineColorBlendAttachmentState;

		// A limited amount of the pipeline state can be changed without recreating the entire pipeline - see VkPipelineDynamicStateCreateInfo.

		buildDescriptorSetLayouts();

		// Get all of the values in the push constant ranges map. 
		std::vector<vk::PushConstantRange> pushConstantRanges;
		std::transform(mPushConstantsMapping.begin(), mPushConstantsMapping.end(), std::back_inserter(pushConstantRanges), [](const auto& val) {return val.second; });

		// Get all of the values in the descriptor set layouts map.
		std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
		std::transform(mDescriptorSetLayoutsMapping.begin(), mDescriptorSetLayoutsMapping.end(), std::back_inserter(descriptorSetLayouts), [](const auto& val) {return val.second; });

		// Encapsulate any descriptor sets and push constant ranges into a pipeline layout.
		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
		pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
		pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());;

		mPipelineLayoutHandle = mDevice->getHandle().createPipelineLayout(pipelineLayoutCreateInfo);

		// Aggregate all of the structures above to create a graphics pipeline.
		vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
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
		graphicsPipelineCreateInfo.subpass = 0;

		mPipelineHandle = mDevice->getHandle().createGraphicsPipeline({}, graphicsPipelineCreateInfo);
	}

	Pipeline::~Pipeline()
	{
		vkDestroyPipeline(mDevice->getHandle(), mPipelineHandle, nullptr);
	}

	vk::PushConstantRange Pipeline::getPushConstantsMember(const std::string &tMemberName) const
	{
		auto it = mPushConstantsMapping.find(tMemberName);

		if (it == mPushConstantsMapping.end())
		{
			throw std::runtime_error("Push constant with name " + tMemberName + " not found");
		}

		return it->second;
	}

	vk::DescriptorSetLayout Pipeline::getDescriptorSetLayout(uint32_t tSet) const
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

	vk::DescriptorPool Pipeline::createCompatibleDescriptorPool(uint32_t tSet, uint32_t tMaxSets)
	{
		// First, make sure that a descriptor set with this index has been recorded.
		if (mDescriptorsMapping.find(tSet) == mDescriptorsMapping.end())
		{
			return VK_NULL_HANDLE;
		}

		// Create a descriptor pool size structure for each of the descriptors in this set.
		std::vector<vk::DescriptorPoolSize> descriptorPoolSizes;
		for (const auto &descriptorSetLayoutBinding : mDescriptorsMapping[tSet])
		{
			vk::DescriptorPoolSize descriptorPoolSize;
			descriptorPoolSize.descriptorCount = descriptorSetLayoutBinding.descriptorCount;
			descriptorPoolSize.type = descriptorSetLayoutBinding.descriptorType;

			descriptorPoolSizes.push_back(descriptorPoolSize);
		}

		// Finally, create the descriptor pool from the list of descriptor pool size structures above.
		vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
		descriptorPoolCreateInfo.maxSets = tMaxSets;
		descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
		descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes.data();

		vk::DescriptorPool descriptorPoolHandle = mDevice->getHandle().createDescriptorPool(descriptorPoolCreateInfo);
		return descriptorPoolHandle;
	}

	vk::PipelineShaderStageCreateInfo Pipeline::buildPipelineShaderStageCreateInfo(const ShaderModuleRef &tShaderModule, vk::ShaderStageFlagBits tShaderStageFlagBits)
	{
		vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfo;
		pipelineShaderStageCreateInfo.module = tShaderModule->getHandle();
		pipelineShaderStageCreateInfo.pName = tShaderModule->getEntryPoints()[0].c_str();
		pipelineShaderStageCreateInfo.pSpecializationInfo = nullptr;
		pipelineShaderStageCreateInfo.stage = tShaderStageFlagBits;

		return pipelineShaderStageCreateInfo;
	}

	void Pipeline::addPushConstantsToGlobalMap(const ShaderModuleRef &tShaderModule, vk::ShaderStageFlagBits tShaderStageFlagBits)
	{
		uint32_t maxPushConstantsSize = mDevice->getPhysicalDeviceProperties().limits.maxPushConstantsSize;

		for (const auto &pushConstant : tShaderModule->getPushConstants())
		{
			// If this push constant already exists in the mapping, simply update its stage flags.
			auto it = mPushConstantsMapping.find(pushConstant.name);
			if (it != mPushConstantsMapping.end() &&
				it->second.offset == pushConstant.offset &&
				it->second.size == pushConstant.size)
			{
				it->second.stageFlags |= tShaderStageFlagBits;
				continue;
			}

			// Otherwise, create a new entry for this push constant.
			vk::PushConstantRange pushConstantRange;
			pushConstantRange.offset = pushConstant.offset;
			pushConstantRange.size = pushConstant.size;
			pushConstantRange.stageFlags = tShaderStageFlagBits;

			mPushConstantsMapping.insert({ pushConstant.name, pushConstantRange });
		}
	}

	void Pipeline::addDescriptorsToGlobalMap(const ShaderModuleRef &tShaderModule, vk::ShaderStageFlagBits tShaderStageFlagBits)
	{
		for (const auto &descriptor : tShaderModule->getDescriptors())
		{
			// for every descriptor found in this shader stage
			uint32_t set = descriptor.layoutSet;

			vk::DescriptorSetLayoutBinding descriptorSetLayoutBinding;
			descriptorSetLayoutBinding.binding = descriptor.layoutBinding;
			descriptorSetLayoutBinding.descriptorCount = descriptor.descriptorCount;
			descriptorSetLayoutBinding.descriptorType = descriptor.descriptorType;
			descriptorSetLayoutBinding.pImmutableSamplers = nullptr;
			descriptorSetLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eAll;

			auto it = mDescriptorsMapping.find(set);
			if (it == mDescriptorsMapping.end())
			{
				std::vector<vk::DescriptorSetLayoutBinding> freshDescriptorSetLayoutBindings = { descriptorSetLayoutBinding };
				mDescriptorsMapping.insert(std::make_pair(set, freshDescriptorSetLayoutBindings));
			}
			else
			{
				// Only add this entry if it doesn't already exist in this set's list of descriptors.
				auto &existingDescriptorSetLayoutBindings = (*it).second;
				auto it = std::find_if(existingDescriptorSetLayoutBindings.begin(), existingDescriptorSetLayoutBindings.end(), 
					[&](const vk::DescriptorSetLayoutBinding &tDescriptorSetLayoutBinding) {
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
			vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
			descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(mDescriptorsMapping[mapping.first].size());
			descriptorSetLayoutCreateInfo.pBindings = mDescriptorsMapping[mapping.first].data();

			vk::DescriptorSetLayout descriptorSetLayout = mDevice->getHandle().createDescriptorSetLayout(descriptorSetLayoutCreateInfo);

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
	
} // namespace graphics