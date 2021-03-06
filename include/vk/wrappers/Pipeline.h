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

#pragma once

#include <iterator>
#include <string>

#include "DescriptorPool.h"
#include "Device.h"
#include "RenderPass.h"
#include "ShaderModule.h"

namespace plume
{

	namespace graphics
	{

		class PipelineLayout
		{
		public:

			//! Factory method for returning a shared PipelineLayout.
			static std::shared_ptr<PipelineLayout> create(const Device& device,
														  const std::vector<vk::PushConstantRange>& push_constant_ranges,
														  const std::vector<vk::DescriptorSetLayout>& descriptor_set_layouts)
			{
				return std::make_shared<PipelineLayout>(device, push_constant_ranges, descriptor_set_layouts);
			}

			PipelineLayout(const Device& device,
						   const std::vector<vk::PushConstantRange>& push_constant_ranges,
						   const std::vector<vk::DescriptorSetLayout>& descriptor_set_layouts)
			{
				vk::PipelineLayoutCreateInfo pipeline_layout_create_info;
				pipeline_layout_create_info.pPushConstantRanges = push_constant_ranges.data();
				pipeline_layout_create_info.pSetLayouts = descriptor_set_layouts.data();
				pipeline_layout_create_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size());
				pipeline_layout_create_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());;

				m_pipeline_layout_handle = m_device_ptr->get_handle().createPipelineLayoutUnique(pipeline_layout_create_info);
			}

		private:

			const Device* m_device_ptr;
			vk::UniquePipelineLayout m_pipeline_layout_handle;
		};

		//! Each pipeline is controlled by a monolithic object created from a description of all of the shader
		//! stages and any relevant fixed-function stages. Linking the whole pipeline together allows the optimization
		//! of shaders based on their inputs/outputs and eliminates expensive draw time state validation.
		//!
		//! Compute pipelines consist of a single static compute shader stage and the pipeline layout. Graphics
		//! pipelines consist of multiple shader stages, multiple fixed-function pipeline stages, and a pipeline
		//! layout. A graphics pipeline is constructed with respect to a particular subpass of a renderpass. Note
		//! that the pipeline must only be used with a render pass that is compatible with the one provided.
		//!
		//! Pipeline cache objects allow the result of pipeline construction to be reused between pipelines and
		//! between runs of an application. 
		//!
		//! Specialization constants are a mechanism whereby constants in a SPIR-V module can have their constant
		//! value specified at the time the pipeline is created.
		class Pipeline
		{
		public:

			Pipeline() = default; 

			Pipeline(const Device& device) :
				m_device_ptr(&device)
			{}

			virtual vk::Pipeline get_handle() const final { return m_pipeline_handle.get(); }

			virtual vk::PipelineLayout get_pipeline_layout_handle() const final { return m_pipeline_layout_handle.get(); }

			//! Returns the pipeline bind point - must be either vk::PipelineBindPoint::eGraphics or vk::PipelineBindPoint::eCompute.
			//! Note that this method is pure virtual and must be overridden by any derived class.
			virtual vk::PipelineBindPoint get_pipeline_bind_point() const = 0;

			//! Returns a push constant range structure that holds information about the push constant with the given name.
			virtual vk::PushConstantRange get_push_constants_member(const std::string& name) const final
			{
				return m_push_constants_mapping.at(name);
			}

			//! Returns a descriptor set layout that holds information about the descriptor set with the given index.
			virtual const vk::DescriptorSetLayout& get_descriptor_set_layout(uint32_t set) const final
			{
				return m_descriptor_set_layouts_mapping.at(set);
			}

			//! Returns `true` if this pipeline owns any descriptor set layouts and `false` otherwise. If a pipeline
			//! is told to infer its own layout during construction, it will examine all of the resources used by its
			//! shader modules and create an appropriate pipeline layout.
			bool has_cached_layouts() { return m_descriptor_set_layouts_mapping.size() > 0; }

			friend std::ostream& operator<<(std::ostream& stream, const Pipeline& pipeline);

		protected:

			//! Builds the struct required to create a new vk::ShaderModule handle. For now, we assume that the entry point for 
			//! each shader module is always "main."
			vk::PipelineShaderStageCreateInfo build_shader_stage_create_info(const std::shared_ptr<ShaderModule>& module);

			//! Given a shader module and shader stage, add all of the module's push constants to the pipeline object's global map.
			void add_push_constants_to_global_map(const std::shared_ptr<ShaderModule>& module);

			//! Given a shader module and shader stage, add all of the module's descriptors to the pipeline object's global map.
			void add_descriptors_to_global_map(const std::shared_ptr<ShaderModule>& module);

			//! Generate all of the descriptor set layout handles.		
			void build_descriptor_set_layouts();

			const Device* m_device_ptr;
			vk::UniquePipeline m_pipeline_handle;
			vk::UniquePipelineLayout m_pipeline_layout_handle;

			std::map<std::string, vk::PushConstantRange> m_push_constants_mapping;
			std::map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>> m_descriptors_mapping;
			std::map<uint32_t, vk::DescriptorSetLayout> m_descriptor_set_layouts_mapping;
		};

		class GraphicsPipeline : public Pipeline
		{
		public:

			//! Helper function for constructing a pipeline color blend attachment state that corresponds to standard alpha blending.
			static vk::PipelineColorBlendAttachmentState create_alpha_blending_attachment_state();

			class Options
			{
			public:

				Options();

				//! Configure color blending properties. Source and destination pixels are combined according to 
				//! the blend operation, quadruplets of source and destination weighting factors determined by
				//! the blend factors, and a blend constant, to obtain a new set of R, G, B, and A values. Each
				//! color attachment used by this pipeline's corresponding subpass can have different blend
				//! settings. The following pseudo-code from the Vulkan Tutorial describes how these per-attachment
				//! blend operations are performed:
				//!
				//! if (blending_enabled) 
				//! {
				//!		final_color.rgb = (src_color_blend_factor * new_color.rgb) <color_blend_op> (dst_color_blend_factor * old_color.rgb);
				//!		final_color.a = (src_alpha_blend_factor * new_color.a) <alpha_blend_op> (dst_alpha_blend_factor * old_color.a);
				//! }
				//! else 
				//! {
				//!		final_color = newColor;
				//! }
				//! final_color = final_color & color_write_mask;
				Options& color_blend_attachment_states(const std::vector<vk::PipelineColorBlendAttachmentState>& color_blend_attachment_states)
				{
					// TODO: right now, this doesn't do anything.
					m_color_blend_attachment_states = color_blend_attachment_states; return *this;
				}

				//! Set the logical operation for all framebuffer attachments. Note that if a logical operation is 
				//! enabled, this will override (disable) all per-attachment blend states. Logical operations are 
				//! applied only for signed/unsigned integer and normalized integer framebuffers. They are not applied 
				//! to floating-point/sRGB format color attachments.
				Options& logic_op(vk::LogicOp logic_op)
				{
					m_color_blend_state_create_info.logicOp = logic_op;
					m_color_blend_state_create_info.logicOpEnable = VK_TRUE;
					return *this;
				}

				//! Enable depth testing.
				Options& depth_test_enabled(bool enabled = true) { m_depth_stencil_state_create_info.depthTestEnable = enabled; return *this; }

				//! Enable stencil testing.
				Options& stencil_test_enable(bool enabled = true) { m_depth_stencil_state_create_info.stencilTestEnable = enabled; return *this; }

				//! Enable both depth and stencil testing.
				Options& depth_stencil_tests_enabled(bool enabled = true)
				{
					m_depth_stencil_state_create_info.depthTestEnable = enabled;
					m_depth_stencil_state_create_info.stencilTestEnable = enabled;
					return *this;
				}

				//! Set the operation used during depth testing.
				Options& depth_compare_op(vk::CompareOp op) { m_depth_stencil_state_create_info.depthCompareOp = op; return *this; }

				//! Define the range of values used in the depth bounds test.
				Options& depth_bounds(float min, float max)
				{
					m_depth_stencil_state_create_info.depthBoundsTestEnable = VK_TRUE;
					m_depth_stencil_state_create_info.minDepthBounds = min;
					m_depth_stencil_state_create_info.maxDepthBounds = max;
					return *this;
				}

				//! Set the number of control points per patch - note that this only matters if the pipeline contains a tessellation 
				//! control and tessellation evaluation shader.
				Options& patch_control_points(uint32_t control_points) { m_tessellation_state_create_info.patchControlPoints = control_points; return *this; }

				//! A limited amount of the pipeline state can be changed without recreating the entire pipeline.
				Options& dynamic_states(const std::vector<vk::DynamicState>& dynamic_states) { m_dynamic_states = dynamic_states; return *this; }

				//! Enable or disable primitive restart.
				Options& primitive_restart_enabled(bool enabled = true) { m_input_assembly_state_create_info.primitiveRestartEnable = enabled; return *this; }

				//! Describe the type of geometry that will be drawn, i.e. triangles, lines, points, etc.
				Options& primitive_topology(vk::PrimitiveTopology primitive_topology) { m_input_assembly_state_create_info.topology = primitive_topology; return *this; }

				//! Set the number of samples per pixel used in rasterization. Multisampling (MSAA) is one method for achieving
				//! full-screen antialiasing (FSAA). Each sub-pixel element will have its own unique depth data, but the color
				//! is only calculated once.
				Options& samples(uint32_t sample_count) { m_multisample_state_create_info.rasterizationSamples = utils::sample_count_to_flags(sample_count); return *this; }

				Options& min_sample_shading(float min_sample_shading)
				{
					m_multisample_state_create_info.minSampleShading = min_sample_shading;
					m_multisample_state_create_info.sampleShadingEnable = VK_TRUE;
					return *this;
				}

				//! Configure frontface/backface culling.
				Options& cull_mode(vk::CullModeFlags cull_mode_flags) { m_rasterization_state_create_info.cullMode = cull_mode_flags; return *this; }

				/*
				 *
				 * Some useful shortcuts - all of these call `cull_mode()` and simply fill out the first parameter.
				 *
				 */
				Options& cull_back() { return cull_mode(vk::CullModeFlagBits::eBack); }
				Options& cull_front() { return cull_mode(vk::CullModeFlagBits::eFront); }
				Options& cull_front_and_back() { return cull_mode(vk::CullModeFlagBits::eFrontAndBack); }

				//! Sets the winding mode to counter-clockwise. 
				Options& front_face_ccw() { m_rasterization_state_create_info.frontFace = vk::FrontFace::eCounterClockwise; return *this; }

				//! Sets the winding mode to clockwise.
				Options& front_face_cw() { m_rasterization_state_create_info.frontFace = vk::FrontFace::eClockwise; return *this; }

				//! Set the line width - only applies if the primitive topology is set to a vk::PrimitiveTopology::eLine* variant.
				Options& line_width(float line_width) { m_rasterization_state_create_info.lineWidth = line_width; return *this; }

				//! Set the polygon mode, i.e. vk::PolygonMode::eFill or vk::PolygonMode::eLines.
				Options& polygon_mode(vk::PolygonMode polygon_mode) { m_rasterization_state_create_info.polygonMode = polygon_mode; return *this; }

				/*
				 *
				 * Some useful shortcuts - all of these call `polygon_mode()` and simply fill out the first parameter.
				 *
				 */
				Options& filled() { return polygon_mode(vk::PolygonMode::eFill); }
				Options& wireframe() { return polygon_mode(vk::PolygonMode::eLine); }
				Options& points() { return polygon_mode(vk::PolygonMode::ePoint); }

				//! Enable rasterizer discard, which forces the pipeline to ignore the entire fragment processing stage.
				Options& rasterizer_discard_enabled(bool enabled = true) { m_rasterization_state_create_info.rasterizerDiscardEnable = enabled; return *this; }

				//! Vertex input binding descriptions tell the implementation how to fetch the vertex data from the GPU once it has 
				//! been uploaded. It describes the rate at which data will be loaded from memory (per-vertex or per-instance). It also
				//! specifies the number of bytes between data entries.
				Options& vertex_input_binding_descriptions(const std::vector<vk::VertexInputBindingDescription>& vertex_input_binding_descriptions) { m_vertex_input_binding_descriptions = vertex_input_binding_descriptions; return *this; }

				//! Vertex input attribute descriptions describe the size, location, and binding index of each vertex attribute. 
				Options& vertex_input_attribute_descriptions(const std::vector<vk::VertexInputAttributeDescription>& vertex_input_attribute_descriptions) { m_vertex_input_attribute_descriptions = vertex_input_attribute_descriptions; return *this; }

				//! Set the region of the framebuffer that the output will be rendered to. Some graphics cards support multiple
				//! viewports/scissors, but doing so requires a special logical device feature.
				Options& viewports(const std::vector<vk::Viewport>& viewports) { m_viewports = viewports; return *this; }

				//! Set the rectangular regions of the framebuffer output that will be visible.
				Options& scissors(const std::vector<vk::Rect2D>& scissors) { m_scissors = scissors; return *this; }

				//! Add a shader stage to the pipeline. Note that all graphics pipeline objects must contain a vertex shader.
				Options& attach_shader_stages(const std::vector<std::shared_ptr<ShaderModule>>& modules) { m_shader_stages = modules; return *this; }

				//! Specify which subpass of the render pass that this pipeline will be associated with.
				Options& subpass_index(uint32_t index) { m_subpass_index = index; return *this; }

			private:

				vk::PipelineColorBlendStateCreateInfo		m_color_blend_state_create_info;	// TODO: this needs to be re-worked.
				vk::PipelineDepthStencilStateCreateInfo		m_depth_stencil_state_create_info;
				vk::PipelineInputAssemblyStateCreateInfo	m_input_assembly_state_create_info;
				vk::PipelineMultisampleStateCreateInfo		m_multisample_state_create_info;
				vk::PipelineRasterizationStateCreateInfo	m_rasterization_state_create_info;
				vk::PipelineTessellationStateCreateInfo		m_tessellation_state_create_info;

				// For vk::Pipeline* structs that take a pointer to a struct(s) as a parameter, we need to actually make a 
				// copy of those structs here, so as to avoid passing an invalid pointer to a temporary variable. For example, 
				// a user could call `viewports()` with an initializer list in-line. However, we need the pointer to that data
				// to remain valid until AFTER the pipeline has been created.

				std::vector<vk::PipelineColorBlendAttachmentState>	m_color_blend_attachment_states;
				std::vector<vk::DynamicState>						m_dynamic_states;
				std::vector<vk::VertexInputBindingDescription>		m_vertex_input_binding_descriptions;
				std::vector<vk::VertexInputAttributeDescription>	m_vertex_input_attribute_descriptions;
				std::vector<vk::Viewport>							m_viewports;
				std::vector<vk::Rect2D>								m_scissors;

				std::vector<std::shared_ptr<ShaderModule>> m_shader_stages;
				uint32_t m_subpass_index;

				friend class GraphicsPipeline;
			};

			GraphicsPipeline() = default; 

			GraphicsPipeline(const Device& device, const RenderPass& render_pass, const Options& options = Options());

			vk::PipelineBindPoint get_pipeline_bind_point() const override { return vk::PipelineBindPoint::eGraphics; }

			//! Returns `true` if the GraphicsPipeline contains a vertex shader stage and `false` otherwise.
			bool has_vertex() const { return m_shader_stage_active_mapping.at(vk::ShaderStageFlagBits::eVertex); }

			//! Returns `true` if the GraphicsPipeline contains a tessellation control shader stage and `false` otherwise.
			bool has_tessellation_control() const { return m_shader_stage_active_mapping.at(vk::ShaderStageFlagBits::eTessellationControl); }

			//! Returns `true` if the GraphicsPipeline contains a tessellation evaluation shader stage and `false` otherwise.
			bool has_tessellation_evaluation() const { return m_shader_stage_active_mapping.at(vk::ShaderStageFlagBits::eTessellationEvaluation); }

			//! Returns `true` if the GraphicsPipeline contains a geometry shader stage and `false` otherwise.
			bool has_geometry() const { return m_shader_stage_active_mapping.at(vk::ShaderStageFlagBits::eGeometry); }

			//! Returns `true` if the GraphicsPipeline contains a fragment shader stage and `false` otherwise.
			bool has_fragment() const { return m_shader_stage_active_mapping.at(vk::ShaderStageFlagBits::eFragment); }
			
			//! Returns `true` if the GraphicsPipeline was created with the specified `dynamic_state`.
			bool has_dynamic_state(vk::DynamicState dynamic_state) const
			{
				return std::find(m_dynamic_states_active.begin(), m_dynamic_states_active.end(), dynamic_state) != m_dynamic_states_active.end();
			}

		private:

			std::map<vk::ShaderStageFlagBits, bool> m_shader_stage_active_mapping
			{
				{ vk::ShaderStageFlagBits::eVertex, false },
				{ vk::ShaderStageFlagBits::eTessellationControl, false },
				{ vk::ShaderStageFlagBits::eTessellationEvaluation, false },
				{ vk::ShaderStageFlagBits::eGeometry, false },
				{ vk::ShaderStageFlagBits::eFragment, false }
			};

			std::vector<vk::DynamicState> m_dynamic_states_active;
		};

		class ComputePipeline : public Pipeline
		{
		public:

			ComputePipeline() = default; 

			ComputePipeline(const Device& device, const std::shared_ptr<ShaderModule>& compute_shader_module);

			vk::PipelineBindPoint get_pipeline_bind_point() const override { return vk::PipelineBindPoint::eCompute; }

		private:

		};

	} // namespace graphics

} // namespace plume