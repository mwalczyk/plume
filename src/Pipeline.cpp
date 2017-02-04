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

#include "Pipeline.h"

namespace graphics
{
	static const std::string spectrum_uniform_names[] =
	{
		"sp_time",
		"sp_resolution",
		"sp_mouse"
	};

	static const std::string spectrum_input_names[] =
	{
		"sp_position",
		"sp_color",
		"sp_normal",
		"sp_texcoord"
	};

	Pipeline::Options::Options()
	{
		// Set up the default viewport.
		m_viewport.x = 0;
		m_viewport.y = 0;
		m_viewport.width = 640;
		m_viewport.height = 480;
		m_viewport.minDepth = 0.0f;
		m_viewport.maxDepth = 1.0f;

		// Set up the default scissor region.
		m_scissor.extent = { static_cast<uint32_t>(m_viewport.width), static_cast<uint32_t>(m_viewport.height) };
		m_scissor.offset = { 0, 0 };

		// Set up the default input assembly.
		m_primitive_restart = VK_FALSE;
		m_primitive_topology = vk::PrimitiveTopology::eTriangleList;

		// Set up parameters for the default rasterization state.
		m_cull_mode_flags = vk::CullModeFlagBits::eBack;
		m_line_width = 1.0f;
		m_polygon_mode = vk::PolygonMode::eFill;

		// Set up the default pipeline color blend attachment state (no blending).
		vk::PipelineColorBlendAttachmentState no_blending;
		no_blending.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		no_blending.blendEnable = VK_FALSE;
		m_color_blend_attachment_states = { no_blending };
		m_logic_op = vk::LogicOp::eCopy;
		m_logic_op_enabled = VK_FALSE;

		// Turn off depth and stencil testing by default.
		m_depth_test_enabled = VK_FALSE;
		m_stencil_test_enabled = VK_FALSE;
	}
   
	vk::PipelineColorBlendAttachmentState Pipeline::create_alpha_blending_attachment_state()
	{
		vk::PipelineColorBlendAttachmentState color_blend_attachment_state;
		color_blend_attachment_state.alphaBlendOp = vk::BlendOp::eAdd;
		color_blend_attachment_state.blendEnable = VK_TRUE;
		color_blend_attachment_state.colorBlendOp = vk::BlendOp::eAdd;
		color_blend_attachment_state.dstAlphaBlendFactor = vk::BlendFactor::eZero;
		color_blend_attachment_state.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		color_blend_attachment_state.srcAlphaBlendFactor = vk::BlendFactor::eOne;
		color_blend_attachment_state.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	
		return color_blend_attachment_state;
	}

	Pipeline::Pipeline(const DeviceRef& device, const RenderPassRef& render_pass, const Options& options) :
		m_device(device),
		m_render_pass(render_pass)
	{		
		// Group the shader create info structs together.
		std::vector<vk::PipelineShaderStageCreateInfo> shader_stage_create_infos;
		bool found_vertex_shader = false;
		bool found_tessellation_control_shader = false;
		bool found_tessellation_evaluation_shader = false;
		bool found_geometry_shader = false;
		bool found_fragment_shader = false;

		for (const auto& stage : options.m_shader_stages)
		{
			if (stage.second == vk::ShaderStageFlagBits::eVertex) found_vertex_shader = true;
			if (stage.second == vk::ShaderStageFlagBits::eTessellationControl) found_tessellation_control_shader = true;
			if (stage.second == vk::ShaderStageFlagBits::eTessellationEvaluation) found_tessellation_evaluation_shader = true;
			if (stage.second == vk::ShaderStageFlagBits::eGeometry) found_geometry_shader = true;
			if (stage.second == vk::ShaderStageFlagBits::eFragment) found_fragment_shader = true;

			auto shader_stage_info = build_shader_stage_create_info(stage.first, stage.second);
			shader_stage_create_infos.push_back(shader_stage_info);
			add_push_constants_to_global_map(stage.first, stage.second);
			add_descriptors_to_global_map(stage.first, stage.second);
		}
		if (!found_vertex_shader)
		{
			throw std::runtime_error("At least one vertex shader stage is required to build a graphics pipeline");
		}

		/*
		// Describe the format of the vertex data that will be passed to the vertex shader.
		vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info;
		vertex_input_state_create_info.pVertexAttributeDescriptions = options.m_vertex_input_attribute_descriptions.data();
		vertex_input_state_create_info.pVertexBindingDescriptions = options.m_vertex_input_binding_descriptions.data();
		vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(options.m_vertex_input_attribute_descriptions.size());
		vertex_input_state_create_info.vertexBindingDescriptionCount = static_cast<uint32_t>(options.m_vertex_input_binding_descriptions.size());

		// Describe the type of geometry that will be drawn and if primitive restart should be enabled.
		vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info;
		input_assembly_state_create_info.primitiveRestartEnable = options.m_primitive_restart;
		input_assembly_state_create_info.topology = options.m_primitive_topology;

		// Combine the viewport and scissor settings into a viewport state structure.
		vk::PipelineViewportStateCreateInfo viewport_state_create_info;
		viewport_state_create_info.pScissors = &options.m_scissor;
		viewport_state_create_info.pViewports = &options.m_viewport;
		viewport_state_create_info.scissorCount = 1;
		viewport_state_create_info.viewportCount = 1;

		// Configure the rasterizer.
		vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info;
		rasterization_state_create_info.cullMode = options.m_cull_mode_flags;
		rasterization_state_create_info.depthBiasClamp = 0.0f;
		rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
		rasterization_state_create_info.depthBiasEnable = VK_FALSE;
		rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
		rasterization_state_create_info.depthClampEnable = VK_FALSE;
		rasterization_state_create_info.frontFace = vk::FrontFace::eClockwise;
		rasterization_state_create_info.lineWidth = options.m_line_width;
		rasterization_state_create_info.polygonMode = options.m_polygon_mode;
		rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;

		// Configure multisampling (anti-aliasing): for now, disable this feature.
		vk::PipelineMultisampleStateCreateInfo multisample_state_create_info;
		multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
		multisample_state_create_info.alphaToOneEnable = VK_FALSE;
		multisample_state_create_info.minSampleShading = 1.0f;
		multisample_state_create_info.pSampleMask = nullptr;
		multisample_state_create_info.rasterizationSamples = vk::SampleCountFlagBits::e1;
		multisample_state_create_info.sampleShadingEnable = VK_FALSE;

		// Configure depth stencil testing.
		vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_create_info;
		depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_state_create_info.depthCompareOp = vk::CompareOp::eLessOrEqual;
		depth_stencil_state_create_info.depthTestEnable = options.m_depth_test_enabled;
		depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
		depth_stencil_state_create_info.maxDepthBounds = 1.0f;		// This is optional, since the depth bounds test is disabled.
		depth_stencil_state_create_info.minDepthBounds = 0.0f;		// This is optional, since the depth bounds test is disabled.
		depth_stencil_state_create_info.stencilTestEnable = options.m_stencil_test_enabled;
		
		// Configure color blending properties. Source and destination pixels are combined according to 
		// the blend operation, quadruplets of source and destination weighting factors determined by
		// the blend factors, and a blend constant, to obtain a new set of R, G, B, and A values. Each
		// color attachment used by this pipeline's corresponding subpass can have different blend
		// settings. The following pseudo-code from the Vulkan Tutorial describes how these per-attachment
		// blend operations are performed:
		//
		// if (blending_enabled) 
		// {
		//		final_color.rgb = (src_color_blend_factor * new_color.rgb) <color_blend_op> (dst_color_blend_factor * old_color.rgb);
		//		final_color.a = (src_alpha_blend_factor * new_color.a) <alpha_blend_op> (dst_alpha_blend_factor * old_color.a);
		// }
		// else 
		// {
		//		final_color = newColor;
		// }
		// final_color = final_color & color_write_mask;
		//
		// Note that if the logic op below is enabled, this will override (disable) all per-attachment
		// blend states. Logical operations are applied only for signed/unsigned integer and normalized
		// integer framebuffers. They are not applied to floating-point/sRGB format color attachments.
		vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info;
		color_blend_state_create_info.attachmentCount = static_cast<uint32_t>(options.m_color_blend_attachment_states.size());
		color_blend_state_create_info.blendConstants[0] = 0.0f;
		color_blend_state_create_info.blendConstants[1] = 0.0f;
		color_blend_state_create_info.blendConstants[2] = 0.0f;
		color_blend_state_create_info.blendConstants[3] = 0.0f;
		color_blend_state_create_info.logicOp = vk::LogicOp::eCopy;
		color_blend_state_create_info.logicOpEnable = VK_FALSE;		// If true, the logic op described here will override the blend modes for every attached framebuffer.
		color_blend_state_create_info.pAttachments = options.m_color_blend_attachment_states.data();

		// A limited amount of the pipeline state can be changed without recreating the entire pipeline.
		vk::PipelineDynamicStateCreateInfo dynamic_state_create_info;
		dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(options.m_dynamic_states.size());
		dynamic_state_create_info.pDynamicStates = options.m_dynamic_states.data();
		*/
		build_descriptor_set_layouts();

		// Get all of the values in the push constant ranges map. 
		std::vector<vk::PushConstantRange> push_constant_ranges;
		std::transform(m_push_constants_mapping.begin(), m_push_constants_mapping.end(), std::back_inserter(push_constant_ranges), [](const auto& val) {return val.second; });

		// Get all of the values in the descriptor set layouts map.
		std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
		std::transform(m_descriptor_set_layouts_mapping.begin(), m_descriptor_set_layouts_mapping.end(), std::back_inserter(descriptor_set_layouts), [](const auto& val) {return val.second; });

		// Encapsulate any descriptor sets and push constant ranges into a pipeline layout.
		vk::PipelineLayoutCreateInfo pipeline_layout_create_info;
		pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges.data();
		pipeline_layout_create_info.pSetLayouts = descriptor_set_layouts.data();
		pipeline_layout_create_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
		pipeline_layout_create_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());;

		m_pipeline_layout_handle = m_device->get_handle().createPipelineLayout(pipeline_layout_create_info);

		// Aggregate all of the structures above to create a graphics pipeline.
		vk::GraphicsPipelineCreateInfo graphics_pipeline_create_info;
		graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
		graphics_pipeline_create_info.basePipelineIndex = -1;
		graphics_pipeline_create_info.layout = m_pipeline_layout_handle;
		graphics_pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
		graphics_pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
		graphics_pipeline_create_info.pDynamicState = (options.m_dynamic_states.size() > 0) ? &dynamic_state_create_info : nullptr;
		graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
		graphics_pipeline_create_info.pMultisampleState = &multisample_state_create_info;
		graphics_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
		graphics_pipeline_create_info.pStages = shader_stage_create_infos.data();
		graphics_pipeline_create_info.pTessellationState = nullptr;
		graphics_pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
		graphics_pipeline_create_info.pViewportState = &viewport_state_create_info;
		graphics_pipeline_create_info.renderPass = m_render_pass->get_handle();
		graphics_pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stage_create_infos.size());
		graphics_pipeline_create_info.subpass = 0;

		m_pipeline_handle = m_device->get_handle().createGraphicsPipeline({}, graphics_pipeline_create_info);
	}

	Pipeline::~Pipeline()
	{
		m_device->get_handle().destroyPipeline(m_pipeline_handle);
		m_device->get_handle().destroyPipelineLayout(m_pipeline_layout_handle);
	}

	vk::PushConstantRange Pipeline::get_push_constants_member(const std::string& name) const
	{
		auto it = m_push_constants_mapping.find(name);

		if (it == m_push_constants_mapping.end())
		{
			throw std::runtime_error("Push constant with name " + name + " not found");
		}

		return it->second;
	}

	vk::DescriptorSetLayout Pipeline::get_descriptor_set_layout(uint32_t set) const
	{
		auto it = m_descriptor_set_layouts_mapping.find(set);

		if (it == m_descriptor_set_layouts_mapping.end())
		{
			std::ostringstream os;
			os << "Descriptor set layout at set " << set << " not found";
			throw std::runtime_error(os.str());
		}

		return it->second;
	}

	DescriptorPoolRef Pipeline::create_compatible_descriptor_pool(uint32_t set, uint32_t max_sets)
	{
		// First, make sure that a descriptor set with this index has been recorded.
		if (m_descriptors_mapping.find(set) == m_descriptors_mapping.end())
		{
			return VK_NULL_HANDLE;
		}

		// Create a descriptor pool size structure for each of the descriptors in this set.
		std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes;
		for (const auto& descriptor_set_layout_binding : m_descriptors_mapping[set])
		{
			vk::DescriptorPoolSize descriptor_pool_size;
			descriptor_pool_size.descriptorCount = descriptor_set_layout_binding.descriptorCount;
			descriptor_pool_size.type = descriptor_set_layout_binding.descriptorType;

			descriptor_pool_sizes.push_back(descriptor_pool_size);
		}

		// Finally, create the descriptor pool from the list of descriptor pool size structures above.
		return DescriptorPool::create(m_device, descriptor_pool_sizes, max_sets);
	}

	vk::PipelineShaderStageCreateInfo Pipeline::build_shader_stage_create_info(const ShaderModuleRef& module, vk::ShaderStageFlagBits shader_stage_flag_bits)
	{
		vk::PipelineShaderStageCreateInfo shader_stage_create_info;
		shader_stage_create_info.module = module->get_handle();
		shader_stage_create_info.pName = module->get_entry_points()[0].c_str();
		shader_stage_create_info.pSpecializationInfo = nullptr;
		shader_stage_create_info.stage = shader_stage_flag_bits;

		return shader_stage_create_info;
	}

	void Pipeline::add_push_constants_to_global_map(const ShaderModuleRef& module, vk::ShaderStageFlagBits shader_stage_flag_bits)
	{
		uint32_t max_push_constants_size = m_device->get_physical_device_properties().limits.maxPushConstantsSize;

		for (const auto& push_constant : module->get_push_constants())
		{
			// If this push constant already exists in the mapping, simply update its stage flags.
			auto it = m_push_constants_mapping.find(push_constant.name);
			if (it != m_push_constants_mapping.end() &&
				it->second.offset == push_constant.offset &&
				it->second.size == push_constant.size)
			{
				it->second.stageFlags |= shader_stage_flag_bits;
				continue;
			}

			// Otherwise, create a new entry for this push constant.
			vk::PushConstantRange push_constant_range;
			push_constant_range.offset = push_constant.offset;
			push_constant_range.size = push_constant.size;
			push_constant_range.stageFlags = shader_stage_flag_bits;

			m_push_constants_mapping.insert({ push_constant.name, push_constant_range });
		}
	}

	void Pipeline::add_descriptors_to_global_map(const ShaderModuleRef& module, vk::ShaderStageFlagBits shader_stage_flag_bits)
	{
		for (const auto& descriptor : module->get_descriptors())
		{
			// for every descriptor found in this shader stage
			uint32_t set = descriptor.layout_set;

			vk::DescriptorSetLayoutBinding descriptor_set_layout_binding;
			descriptor_set_layout_binding.binding = descriptor.layout_binding;
			descriptor_set_layout_binding.descriptorCount = descriptor.descriptor_count;
			descriptor_set_layout_binding.descriptorType = descriptor.descriptor_type;
			descriptor_set_layout_binding.pImmutableSamplers = nullptr;
			descriptor_set_layout_binding.stageFlags = vk::ShaderStageFlagBits::eAll;

			auto it = m_descriptors_mapping.find(set);
			if (it == m_descriptors_mapping.end())
			{
				std::vector<vk::DescriptorSetLayoutBinding> fresh_descriptor_set_layout_bindings = { descriptor_set_layout_binding };
				m_descriptors_mapping.insert(std::make_pair(set, fresh_descriptor_set_layout_bindings));
			}
			else
			{
				// Only add this entry if it doesn't already exist in this set's list of descriptors.
				auto& existing_descriptor_set_layout_bindings = (*it).second;
				auto it = std::find_if(existing_descriptor_set_layout_bindings.begin(), existing_descriptor_set_layout_bindings.end(),
					[&](const vk::DescriptorSetLayoutBinding &tDescriptorSetLayoutBinding) 
					{
						return tDescriptorSetLayoutBinding.binding == descriptor_set_layout_binding.binding;
					});

				if (it == existing_descriptor_set_layout_bindings.end())
				{
					existing_descriptor_set_layout_bindings.push_back(descriptor_set_layout_binding);
				}
			}
		}			
	}

	void Pipeline::build_descriptor_set_layouts()
	{
		for (const auto& mapping : m_descriptors_mapping)
		{
			vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info;
			descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(m_descriptors_mapping[mapping.first].size());
			descriptor_set_layout_create_info.pBindings = m_descriptors_mapping[mapping.first].data();

			vk::DescriptorSetLayout descriptorSetLayout = m_device->get_handle().createDescriptorSetLayout(descriptor_set_layout_create_info);

			m_descriptor_set_layouts_mapping.insert(std::make_pair(mapping.first, descriptorSetLayout));
		}
	}

	std::ostream& operator<<(std::ostream& stream, const PipelineRef& pipeline)
	{
		stream << "Pipeline object: " << pipeline->m_pipeline_handle << std::endl;

		stream << "Push constants details:" << std::endl;
		for (const auto& mapping : pipeline->m_push_constants_mapping)
		{
			stream << "\tPush constant named: " << mapping.first << ":" << std::endl;
			stream << "\t\tOffset: " << mapping.second.offset << std::endl;
			stream << "\t\tSize: " << mapping.second.size << std::endl;
			stream << "\t\tShader stage flags: " << vk::to_string(mapping.second.stageFlags) << std::endl;
		}

		stream << "Descriptor set details:" << std::endl;
		for (const auto& mapping : pipeline->m_descriptors_mapping)
		{
			stream << "\tDescriptor set #" << mapping.first << ":" << std::endl;
			for (const auto& descriptor_set_layout_binding : mapping.second)
			{
				stream << "\t\tDescriptor at binding: " << descriptor_set_layout_binding.binding << std::endl;
				stream << "\t\t\tDescriptor count: " << descriptor_set_layout_binding.descriptorCount << std::endl;
				stream << "\t\t\tDescriptor type: " << vk::to_string(descriptor_set_layout_binding.descriptorType) << std::endl;
				stream << "\t\t\tShader stage flags: " << vk::to_string(descriptor_set_layout_binding.stageFlags) << std::endl;
			}
		}

		return stream;
	}
	
} // namespace graphics