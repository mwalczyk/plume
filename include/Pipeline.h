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

#include <memory>
#include <vector>
#include <map>
#include <iterator>
#include <string>

#include "Platform.h"
#include "Noncopyable.h"
#include "DescriptorPool.h"
#include "Device.h"
#include "RenderPass.h"
#include "ShaderModule.h"

namespace graphics
{

	class Pipeline;
	using PipelineRef = std::shared_ptr<Pipeline>;

	class Pipeline : public Noncopyable
	{
	public:

		class Options
		{
		public:

			Options();
			
			Options& vertex_input_binding_descriptions(const std::vector<vk::VertexInputBindingDescription>& vertex_input_binding_descriptions) { m_vertex_input_binding_descriptions = vertex_input_binding_descriptions; return *this; }
			Options& vertex_input_attribute_descriptions(const std::vector<vk::VertexInputAttributeDescription>& vertex_input_attribute_descriptions) { m_vertex_input_attribute_descriptions = vertex_input_attribute_descriptions; return *this; }
			Options& viewport(const vk::Viewport& viewport) { m_viewport = viewport; return *this; }
			Options& scissor(const vk::Rect2D& scissor) { m_scissor = scissor; return *this; }
			Options& attach_shader_stage(const ShaderModuleRef& module, vk::ShaderStageFlagBits shader_stage_flag_bits) { m_shader_stages.push_back({ module, shader_stage_flag_bits }); return *this; }
			Options& polygon_mode(vk::PolygonMode polygon_mode) { m_polygon_mode = polygon_mode; return *this; }
			Options& line_width(float line_width) { m_line_width = line_width; return *this; }
			Options& cull_mode(vk::CullModeFlags cull_mode_flags) { m_cull_mode_flags = cull_mode_flags; return *this; }
			Options& primitive_restart(bool primitive_restart) { m_primitive_restart = primitive_restart; return *this; }
			Options& primitive_topology(vk::PrimitiveTopology primitive_topology) { m_primitive_topology = primitive_topology; return *this; }
			Options& depth_test(bool enabled = true) { m_depth_test_enabled = enabled; return *this; }
			Options& stencilTest(bool enabled = true) { m_stencil_test_enabled = enabled; return *this; }

			//! Configure per-attached framebuffer color blending, which determines how new fragments are composited with colors that are already in the framebuffer.
			Options& color_blend_attachment_state(const vk::PipelineColorBlendAttachmentState& color_blend_attachment_state) { m_color_blend_attachment_state = color_blend_attachment_state; return *this; }
		
		private:

			std::vector<vk::VertexInputBindingDescription> m_vertex_input_binding_descriptions;
			std::vector<vk::VertexInputAttributeDescription> m_vertex_input_attribute_descriptions;
			vk::Viewport m_viewport;
			vk::Rect2D m_scissor;
			std::vector<std::pair<ShaderModuleRef, vk::ShaderStageFlagBits>> m_shader_stages;
			vk::PolygonMode m_polygon_mode;
			float m_line_width;
			vk::CullModeFlags m_cull_mode_flags;
			bool m_primitive_restart;
			vk::PrimitiveTopology m_primitive_topology;
			bool m_depth_test_enabled;
			bool m_stencil_test_enabled;
			vk::PipelineColorBlendAttachmentState m_color_blend_attachment_state;

			friend class Pipeline;
		};

		//! Factory method for returning a new PipelineRef.
		static PipelineRef create(const DeviceRef& device, const RenderPassRef& render_pass, const Options& options = Options()) 
		{ 
			return std::make_shared<Pipeline>(device, render_pass, options);
		}
		
		//! Helper function for constructing a vertex input binding description.
		static vk::VertexInputBindingDescription create_vertex_input_binding_description(uint32_t binding, uint32_t stride, vk::VertexInputRate input_rate = vk::VertexInputRate::eVertex);

		//! Helper function for constructing a vertex input attribute description.
		static vk::VertexInputAttributeDescription create_vertex_input_attribute_description(uint32_t binding, vk::Format format, uint32_t location, uint32_t offset);

		//! Helper function for constructing a pipeline color blend attachment state that corresponds to standard alpha blending.
		static vk::PipelineColorBlendAttachmentState create_alpha_blending_attachment_state();

		Pipeline(const DeviceRef& device, const RenderPassRef& render_pass, const Options& options = Options());
		~Pipeline();

		inline vk::Pipeline get_handle() const { return m_pipeline_handle; }
		inline vk::PipelineLayout get_pipeline_layout_handle() const { return m_pipeline_layout_handle; }

		//! Returns a push constant range structure that holds information about the push constant with the given name.
		vk::PushConstantRange get_push_constants_member(const std::string& name) const;

		//! Returns a descriptor set layout that holds information about the descriptor set with the given index.
		vk::DescriptorSetLayout get_descriptor_set_layout(uint32_t set) const;

		//! Given a descriptor set index, create and return a handle to a new descriptor pool whose size matches the combined 
		//! size of all of the descriptors in that set. If there is no descriptor set with the given index, return a null handle.
		DescriptorPoolRef create_compatible_descriptor_pool(uint32_t set, uint32_t max_sets = 1);

		friend std::ostream& operator<<(std::ostream& stream, const PipelineRef& pipeline);

	private:

		vk::PipelineShaderStageCreateInfo build_shader_stage_create_info(const ShaderModuleRef& module, vk::ShaderStageFlagBits shader_stage_flag_bits);

		//! Given a shader module and shader stage, add all of the module's push constants to the pipeline object's global map.
		void add_push_constants_to_global_map(const ShaderModuleRef& module, vk::ShaderStageFlagBits shader_stage_flag_bits);

		//! Given a shader module and shader stage, add all of the module's descriptors to the pipeline object's global map.
		void add_descriptors_to_global_map(const ShaderModuleRef& module, vk::ShaderStageFlagBits shader_stage_flag_bits);

		//! Generate all of the descriptor set layout handles.		
		void build_descriptor_set_layouts();

		DeviceRef m_device;
		RenderPassRef m_render_pass;
		vk::Pipeline m_pipeline_handle;
		vk::PipelineLayout m_pipeline_layout_handle;
		std::map<std::string, vk::PushConstantRange> m_push_constants_mapping;
		std::map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>> m_descriptors_mapping;
		std::map<uint32_t, vk::DescriptorSetLayout> m_descriptor_set_layouts_mapping;
	};

} // namespace graphics