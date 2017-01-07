#include "Pipeline.h"

namespace vk
{

	static std::vector<char> readFile(const std::string &tFileName)
	{
		// Start reading at the end of the file to determine file size.
		std::ifstream file(tFileName, std::ios::ate | std::ios::binary);
		
		if (!file.is_open())
		{
			std::cerr << "Failed to open file " << tFileName << "\n";
		}

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> fileContents(fileSize);

		file.seekg(0);
		file.read(fileContents.data(), fileSize);

		file.close();

		std::cout << "Successfully read " << fileSize << " bytes from file: " << tFileName << "\n";

		return fileContents;
	}

	Pipeline::Options::Options()
	{

	}

	Pipeline::Pipeline(const DeviceRef &tDevice, const Options &tOptions) :
		mDevice(tDevice)
	{
		auto vertShaderSrc = readFile("assets/shaders/vert.spv");
		auto fragShaderSrc = readFile("assets/shaders/frag.spv");

		// Shader module objects are only required during the pipeline creation process.
		VkShaderModule vertShaderModule{ VK_NULL_HANDLE };
		VkShaderModule fragShaderModule{ VK_NULL_HANDLE };
		createShaderModule(vertShaderSrc, &vertShaderModule);
		createShaderModule(fragShaderSrc, &fragShaderModule);

		// Assign shader modules to specific shader stages.
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.flags = 0;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";
		vertShaderStageInfo.pNext = nullptr;
		vertShaderStageInfo.pSpecializationInfo = nullptr;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.flags = 0;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";
		fragShaderStageInfo.pNext = nullptr;
		fragShaderStageInfo.pSpecializationInfo = nullptr;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

		// Group the create info structures together.
		VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[] = { vertShaderStageInfo, fragShaderStageInfo };

		// Describe the format of the vertex data that will be passed to the vertex shader.
		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
		vertexInputStateCreateInfo.flags = 0;
		vertexInputStateCreateInfo.pNext = nullptr;
		vertexInputStateCreateInfo.pVertexAttributeDescriptions = nullptr;
		vertexInputStateCreateInfo.pVertexBindingDescriptions = nullptr;
		vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
		vertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;

		// Describe the type of geometry that will be drawn and if primitive restart should be enabled.
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
		inputAssemblyStateCreateInfo.flags = 0;
		inputAssemblyStateCreateInfo.pNext = nullptr;
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

		// For now, create an empty pipeline layout.
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
		pipelineLayoutCreateInfo.pSetLayouts = nullptr;
		pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		pipelineLayoutCreateInfo.setLayoutCount = 0;
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		auto result = vkCreatePipelineLayout(mDevice->getHandle(), &pipelineLayoutCreateInfo, nullptr, &mPipelineLayoutHandle);
		assert(result == VK_SUCCESS);

		std::cout << "Successfully created pipeline layout\n";

		/*


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
		graphicsPipelineCreateInfo.renderPass = mRenderPass;
		graphicsPipelineCreateInfo.stageCount = 2;
		graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineCreateInfo.subpass = 0;

		result = vkCreateGraphicsPipelines(mDevice->getHandle(), VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &mPipelineHandle);
		assert(result == VK_SUCCESS);

		std::cout << "Successfully created pipeline\n";

		*/

		vkDestroyShaderModule(mDevice->getHandle(), vertShaderModule, nullptr);
		vkDestroyShaderModule(mDevice->getHandle(), fragShaderModule, nullptr);
	}

	Pipeline::~Pipeline()
	{
		vkDestroyPipeline(mDevice->getHandle(), mPipelineHandle, nullptr);
	}

	void Pipeline::createRenderPass()
	{
		// For now, we simply have a single color attachment, represented by one of the images from the swapchain.
		VkAttachmentDescription attachmentDescription = {};
		attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachmentDescription.flags = 0;
		attachmentDescription.format = mSwapchainImageFormat;
		attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;		// clear the existing contents of the attachment prior to rendering
		attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;			// no multisampling
		attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;	// rendered contents will be stored in memory for later reads

		// Create a reference to the attachment described above
		VkAttachmentReference attachmentReference = {};
		attachmentReference.attachment = 0;
		attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// A single render pass can consist of multiple subpasses (subsequent rendering operations).
		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &attachmentReference;

		// Create a subpass dependency.
		VkSubpassDependency subpassDependency = {};
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.srcAccessMask = 0;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		// Create a render pass with the information above.
		VkRenderPassCreateInfo renderPassCreateInfo = {};
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.dependencyCount = 1;
		renderPassCreateInfo.flags = 0;
		renderPassCreateInfo.pAttachments = &attachmentDescription;
		renderPassCreateInfo.pDependencies = &subpassDependency;
		renderPassCreateInfo.pNext = nullptr;
		renderPassCreateInfo.pSubpasses = &subpassDescription;
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.subpassCount = 1;

		auto result = vkCreateRenderPass(mDevice->getHandle(), &renderPassCreateInfo, nullptr, &mRenderPassHandle);
		assert(result == VK_SUCCESS);

		std::cout << "Successfully created render pass\n";
	}

	void Pipeline::createShaderModule(const std::vector<char> &tSource, VkShaderModule *tShaderModule)
	{
		VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
		shaderModuleCreateInfo.codeSize = tSource.size();
		shaderModuleCreateInfo.pCode = (uint32_t*)tSource.data();
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		auto result = vkCreateShaderModule(mDevice->getHandle(), &shaderModuleCreateInfo, nullptr, tShaderModule);
		assert(result == VK_SUCCESS);

		std::cout << "Successfully created shader module\n";
	}

} // namespace vk