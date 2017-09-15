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
		// Set up the default color blend state.
		static vk::PipelineColorBlendAttachmentState default_color_blend_attachment;
		default_color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		default_color_blend_attachment.blendEnable = VK_FALSE;

		m_color_blend_state_create_info.attachmentCount = 1;
		m_color_blend_state_create_info.blendConstants[0] = 0.0f;
		m_color_blend_state_create_info.blendConstants[1] = 0.0f;
		m_color_blend_state_create_info.blendConstants[2] = 0.0f;
		m_color_blend_state_create_info.blendConstants[3] = 0.0f;
		m_color_blend_state_create_info.logicOp = vk::LogicOp::eCopy;
		m_color_blend_state_create_info.logicOpEnable = VK_FALSE;		
		m_color_blend_state_create_info.pAttachments = &default_color_blend_attachment;
		
		// Set up the default depth stencil state.
		m_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
		m_depth_stencil_state_create_info.depthCompareOp = vk::CompareOp::eLessOrEqual;
		m_depth_stencil_state_create_info.depthTestEnable = VK_FALSE;
		m_depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
		m_depth_stencil_state_create_info.maxDepthBounds = 1.0f;
		m_depth_stencil_state_create_info.minDepthBounds = 0.0f;
		m_depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
		
		// Set up the default input assembly state.
		m_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;
		m_input_assembly_state_create_info.topology = vk::PrimitiveTopology::eTriangleList;

		// Set up the default multisample state.
		m_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
		m_multisample_state_create_info.alphaToOneEnable = VK_FALSE;
		m_multisample_state_create_info.minSampleShading = 1.0f;
		m_multisample_state_create_info.pSampleMask = nullptr;
		m_multisample_state_create_info.rasterizationSamples = vk::SampleCountFlagBits::e1;
		m_multisample_state_create_info.sampleShadingEnable = VK_FALSE;

		// Set up the default rasterization state.
		m_rasterization_state_create_info.cullMode = vk::CullModeFlagBits::eNone;
		m_rasterization_state_create_info.depthBiasClamp = 0.0f;
		m_rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
		m_rasterization_state_create_info.depthBiasEnable = VK_FALSE;
		m_rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
		m_rasterization_state_create_info.depthClampEnable = VK_FALSE;
		m_rasterization_state_create_info.frontFace = vk::FrontFace::eClockwise;
		m_rasterization_state_create_info.lineWidth = 1.0f;
		m_rasterization_state_create_info.polygonMode = vk::PolygonMode::eFill;
		m_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
			
		// Set up the default tessellation state.
		m_tessellation_state_create_info.patchControlPoints = 3;

		// Set up the default dynamic state.
		static std::vector<vk::DynamicState> default_dynamic_state = { 
			vk::DynamicState::eLineWidth
		};
		m_dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(default_dynamic_state.size());
		m_dynamic_state_create_info.pDynamicStates = default_dynamic_state.data();

		// Set up the default viewport state.
		m_viewports = { 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f };
		m_scissors = { { 0, 0 }, { 640, 480 } };
		
		// Set the default subpass index.
		m_subpass_index = 0;
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

	Pipeline::Pipeline(DeviceWeakRef device, const RenderPassRef& render_pass, const Options& options) :
		m_device(device),
		m_render_pass(render_pass)
	{		
		DeviceRef device_shared = m_device.lock();

		// Group the shader create info structs together.
		std::vector<vk::PipelineShaderStageCreateInfo> shader_stage_create_infos;
		bool found_vertex_shader = false;
		bool found_tessellation_control_shader = false;
		bool found_tessellation_evaluation_shader = false;
		bool found_geometry_shader = false;
		bool found_fragment_shader = false;

		for (const auto& stage : options.m_shader_stages)
		{
			if (stage->get_stage() == vk::ShaderStageFlagBits::eVertex) found_vertex_shader = true;
			if (stage->get_stage() == vk::ShaderStageFlagBits::eTessellationControl) found_tessellation_control_shader = true;
			if (stage->get_stage() == vk::ShaderStageFlagBits::eTessellationEvaluation) found_tessellation_evaluation_shader = true;
			if (stage->get_stage() == vk::ShaderStageFlagBits::eGeometry) found_geometry_shader = true;
			if (stage->get_stage() == vk::ShaderStageFlagBits::eFragment) found_fragment_shader = true;

			auto shader_stage_info = build_shader_stage_create_info(stage);
			shader_stage_create_infos.push_back(shader_stage_info);
			add_push_constants_to_global_map(stage);
			add_descriptors_to_global_map(stage);
		}
		if (!found_vertex_shader)
		{
			throw std::runtime_error("At least one vertex shader stage is required to build a graphics pipeline");
		}
		if (options.m_input_assembly_state_create_info.topology == vk::PrimitiveTopology::ePatchList &&
			!(found_tessellation_control_shader && found_tessellation_evaluation_shader))	// TODO: are tessellation evaluation shaders optional?
		{
			throw std::runtime_error("No tessellation control and/or tessellation evaluation shader were found, but the primitive topology\
									  is set to vk::PrimitiveTopology::ePatchList");
		}
		
		vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info;
		vertex_input_state_create_info.pVertexAttributeDescriptions = options.m_vertex_input_attribute_descriptions.data();
		vertex_input_state_create_info.pVertexBindingDescriptions = options.m_vertex_input_binding_descriptions.data();
		vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(options.m_vertex_input_attribute_descriptions.size());
		vertex_input_state_create_info.vertexBindingDescriptionCount = static_cast<uint32_t>(options.m_vertex_input_binding_descriptions.size());

		vk::PipelineViewportStateCreateInfo viewport_state_create_info;
		viewport_state_create_info.pScissors = options.m_scissors.data();
		viewport_state_create_info.pViewports = options.m_viewports.data();
		viewport_state_create_info.scissorCount = static_cast<uint32_t>(options.m_scissors.size());
		viewport_state_create_info.viewportCount = static_cast<uint32_t>(options.m_viewports.size());

		build_descriptor_set_layouts();

		// Get all of the values in the push constant ranges map. 
		std::vector<vk::PushConstantRange> push_constant_ranges;
		std::transform(m_push_constants_mapping.begin(), m_push_constants_mapping.end(), std::back_inserter(push_constant_ranges), [](const auto& val) { return val.second; });

		// Get all of the values in the descriptor set layouts map.
		std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
		std::transform(m_descriptor_set_layouts_mapping.begin(), m_descriptor_set_layouts_mapping.end(), std::back_inserter(descriptor_set_layouts), [](const auto& val) { return val.second; });

		// Encapsulate any descriptor sets and push constant ranges into a pipeline layout.
		vk::PipelineLayoutCreateInfo pipeline_layout_create_info;
		pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges.data();
		pipeline_layout_create_info.pSetLayouts = descriptor_set_layouts.data();
		pipeline_layout_create_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
		pipeline_layout_create_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());;

		m_pipeline_layout_handle = device_shared->get_handle().createPipelineLayout(pipeline_layout_create_info);

		// Aggregate all of the structures above to create a graphics pipeline.
		vk::GraphicsPipelineCreateInfo graphics_pipeline_create_info;
		graphics_pipeline_create_info.basePipelineHandle = vk::Pipeline{};
		graphics_pipeline_create_info.basePipelineIndex = -1;
		graphics_pipeline_create_info.layout = m_pipeline_layout_handle;
		graphics_pipeline_create_info.pColorBlendState = &options.m_color_blend_state_create_info;
		graphics_pipeline_create_info.pDepthStencilState = &options.m_depth_stencil_state_create_info;
		graphics_pipeline_create_info.pDynamicState = (options.m_dynamic_state_create_info.dynamicStateCount > 0) ? &options.m_dynamic_state_create_info : nullptr;
		graphics_pipeline_create_info.pInputAssemblyState = &options.m_input_assembly_state_create_info;
		graphics_pipeline_create_info.pMultisampleState = &options.m_multisample_state_create_info;
		graphics_pipeline_create_info.pRasterizationState = &options.m_rasterization_state_create_info;
		graphics_pipeline_create_info.pStages = shader_stage_create_infos.data();
		graphics_pipeline_create_info.pTessellationState = &options.m_tessellation_state_create_info;
		graphics_pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
		graphics_pipeline_create_info.pViewportState = &viewport_state_create_info;
		graphics_pipeline_create_info.renderPass = m_render_pass->get_handle();
		graphics_pipeline_create_info.stageCount = static_cast<uint32_t>(shader_stage_create_infos.size());
		graphics_pipeline_create_info.subpass = options.m_subpass_index;

		m_pipeline_handle = device_shared->get_handle().createGraphicsPipeline({}, graphics_pipeline_create_info);
	}

	Pipeline::~Pipeline()
	{
		DeviceRef device_shared = m_device.lock();

		device_shared->get_handle().destroyPipeline(m_pipeline_handle);
		device_shared->get_handle().destroyPipelineLayout(m_pipeline_layout_handle);
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

	vk::PipelineShaderStageCreateInfo Pipeline::build_shader_stage_create_info(const ShaderModuleRef& module)
	{
		vk::PipelineShaderStageCreateInfo shader_stage_create_info;
		shader_stage_create_info.module = module->get_handle();
		shader_stage_create_info.pName = module->get_entry_points()[0].c_str();
		shader_stage_create_info.pSpecializationInfo = nullptr;
		shader_stage_create_info.stage = module->get_stage();

		return shader_stage_create_info;
	}

	void Pipeline::add_push_constants_to_global_map(const ShaderModuleRef& module)
	{
		DeviceRef device_shared = m_device.lock();

		uint32_t max_push_constants_size = device_shared->get_physical_device_properties().limits.maxPushConstantsSize;

		for (const auto& push_constant : module->get_push_constants())
		{
			// If this push constant already exists in the mapping, simply update its stage flags.
			auto it = m_push_constants_mapping.find(push_constant.name);
			if (it != m_push_constants_mapping.end() &&
				it->second.offset == push_constant.offset &&
				it->second.size == push_constant.size)
			{
				// TODO: this isn't working
				// it->second.stageFlags |= module->get_stage();
				continue;
			}

			// Otherwise, create a new entry for this push constant.
			vk::PushConstantRange push_constant_range;
			push_constant_range.offset = push_constant.offset;
			push_constant_range.size = push_constant.size;
			push_constant_range.stageFlags = vk::ShaderStageFlagBits::eAll; module->get_stage();

			m_push_constants_mapping.insert({ push_constant.name, push_constant_range });
		}
	}

	void Pipeline::add_descriptors_to_global_map(const ShaderModuleRef& module)
	{
		for (const auto& descriptor : module->get_descriptors())
		{
			// for every descriptor found in this shader stage
			uint32_t set = descriptor.set;

			auto it = m_descriptors_mapping.find(set);
			if (it == m_descriptors_mapping.end())
			{
				std::vector<vk::DescriptorSetLayoutBinding> fresh_descriptor_set_layout_bindings = { descriptor.layout_binding };
				m_descriptors_mapping.insert(std::make_pair(set, fresh_descriptor_set_layout_bindings));
			}
			else
			{
				// Only add this entry if it doesn't already exist in this set's list of descriptors.
				auto& existing_descriptor_set_layout_bindings = (*it).second;
				auto it = std::find_if(existing_descriptor_set_layout_bindings.begin(), existing_descriptor_set_layout_bindings.end(),
					[&](const vk::DescriptorSetLayoutBinding &tDescriptorSetLayoutBinding) 
					{
						return tDescriptorSetLayoutBinding.binding == descriptor.layout_binding.binding;
					});

				if (it == existing_descriptor_set_layout_bindings.end())
				{
					existing_descriptor_set_layout_bindings.push_back(descriptor.layout_binding);
				}
			}
		}			
	}

	void Pipeline::build_descriptor_set_layouts()
	{
		DeviceRef device_shared = m_device.lock();

		// Iterate through the map of descriptors, which maps descriptor set IDs (i.e. 0, 1, 2) to
		// a list of descriptors (i.e. uniform buffers, samplers), and create a descriptor set layout
		// for each set.
		for (const auto& mapping : m_descriptors_mapping)
		{
			vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info;
			descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(mapping.second.size());
			descriptor_set_layout_create_info.pBindings = mapping.second.data();

			vk::DescriptorSetLayout descriptor_set_layout = device_shared->get_handle().createDescriptorSetLayout(descriptor_set_layout_create_info);

			m_descriptor_set_layouts_mapping.insert(std::make_pair(mapping.first, descriptor_set_layout));
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