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

	ShaderModule::ShaderModule(const DeviceRef &tDevice, const std::string &tFilePath) :
		mDevice(tDevice)
	{
		auto shaderSrc = readFile(tFilePath);
		
		if (shaderSrc.size() % 4)
		{
			throw std::runtime_error("Shader source code is an invalid size");
		}

		auto pCode = reinterpret_cast<const uint32_t*>(shaderSrc.data());

		// Store the SPIR-V code for reflection.
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

		// Get all of the push constants blocks and their members (currently, Vulkan only supports one block).
		for (const auto &resource : shaderResources.push_constant_buffers)
		{
			PushConstantsBlock pushConstantsBlock;
			pushConstantsBlock.layout = compilerGlsl.get_decoration(resource.id, spv::Decoration::DecorationLocation);
			pushConstantsBlock.totalSize = compilerGlsl.get_declared_struct_size(compilerGlsl.get_type(resource.base_type_id));
			pushConstantsBlock.name = resource.name;

			mEntryPoints = compilerGlsl.get_entry_points();

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
				uint32_t size = 0;
				switch (type.basetype)
				{
				case spirv_cross::SPIRType::Float:
					size = rows * columns * sizeof(float);
					break;
				case spirv_cross::SPIRType::Int:
					size = rows * columns * sizeof(int);
				default:
					break;
				}				

				auto offset = range.offset;
				auto name = compilerGlsl.get_member_name(resource.base_type_id, range.index);
			
				pushConstantsMembers.emplace_back(index, size, offset, name);
			}

			mPushConstantsMapping.emplace(pushConstantsBlock, pushConstantsMembers);
		}

		for (const auto &resource : shaderResources.stage_inputs)
		{
			std::cout << "Input resource ID: " << resource.id << std::endl;
			std::cout << "\tName: " << resource.name << std::endl;
			std::cout << "\tLayout: " << compilerGlsl.get_decoration(resource.id, spv::Decoration::DecorationLocation) << std::endl;
		}

		for (const auto &resource : shaderResources.stage_outputs)
		{
			std::cout << "Output resource ID: " << resource.id << std::endl;
			std::cout << "\tName: " << resource.name << std::endl;
			std::cout << "\tLayout: " << compilerGlsl.get_decoration(resource.id, spv::Decoration::DecorationLocation) << std::endl;
		}
	}

	Pipeline::Options::Options()
	{

	}

	Pipeline::Pipeline(const DeviceRef &tDevice, const RenderPassRef &tRenderPass, const Options &tOptions) :
		mDevice(tDevice),
		mRenderPass(tRenderPass),
		mPushConstantRanges(tOptions.mPushConstantRanges)
	{
		// Shader module objects are only required during the pipeline creation process.
		auto vertShaderModule = ShaderModule::create(mDevice, "../assets/shaders/vert.spv");
		auto fragShaderModule = ShaderModule::create(mDevice, "../assets/shaders/frag.spv");

		uint32_t maxPushConstantsSize = mDevice->getPhysicalDeviceProperties().limits.maxPushConstantsSize;
		std::cout << "Maximum size of push constants: " << maxPushConstantsSize << std::endl;
		
		// Assign shader modules to specific shader stages.
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.module = vertShaderModule->getHandle();
		vertShaderStageInfo.pName = "main";
		vertShaderStageInfo.pSpecializationInfo = nullptr; 
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.module = fragShaderModule->getHandle();
		fragShaderStageInfo.pName = "main";
		fragShaderStageInfo.pSpecializationInfo = nullptr;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

		// Group the create info structures together.
		VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[] = { vertShaderStageInfo, fragShaderStageInfo };

		// Describe the format of the vertex data that will be passed to the vertex shader.
		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
		vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;
		vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
		vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
		vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;

		// Describe the type of geometry that will be drawn and if primitive restart should be enabled.
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
		inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
		inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		// Set up a viewport.
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = 640;
		viewport.height = 480;
		viewport.minDepth = 0.0f;	
		viewport.maxDepth = 1.0f;

		// Set up a fullscreen scissor rectangle.
		VkRect2D scissor = {};
		scissor.extent = { 640, 480 };
		scissor.offset = { 0, 0 };

		// Combine the viewport and scissor settings into a viewport state structure.
		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
		viewportStateCreateInfo.pScissors = &scissor;
		viewportStateCreateInfo.pViewports = &viewport;
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

		// Encapsulate any descriptor sets and push constant ranges into a pipeline layout.
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.pPushConstantRanges = mPushConstantRanges.data();
		pipelineLayoutCreateInfo.pSetLayouts = nullptr;
		pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(mPushConstantRanges.size());
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
		graphicsPipelineCreateInfo.pStages = pipelineShaderStageCreateInfos;
		graphicsPipelineCreateInfo.pTessellationState = nullptr;
		graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
		graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
		graphicsPipelineCreateInfo.renderPass = mRenderPass->getHandle();
		graphicsPipelineCreateInfo.stageCount = 2;
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

} // namespace vk