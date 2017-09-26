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

#include "DescriptorPool.h"

namespace plume
{

	namespace graphics
	{

		void DescriptorSetLayoutBuilder::add_binding(vk::DescriptorType type, uint32_t binding, uint32_t count, vk::ShaderStageFlags stages)
		{
			if (!m_is_recording)
			{
				throw std::runtime_error("Adding a new binding must be called between `begin_descriptor_set_record()` and `end_descriptor_set_record()`");
			}

			m_descriptor_sets_mapping[m_current_set].push_back({
				binding,		// binding (as it appears in the shader code)
				type,			// descriptor type (i.e. vk::DescriptorType::eUniformBuffer)
				count,			// descriptor count (if the descriptor is an array)
				stages,			// shader usage stages
				nullptr			// immutable samplers 
			});
		}

		std::vector<vk::DescriptorSetLayout> DescriptorSetLayoutBuilder::build_layouts() const
		{
			if (m_is_recording)
			{
				throw std::runtime_error("The LayoutBuilder is still in a recording state - call `end_descriptor_set_record()` before `build_layouts()`.");
			}

			std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;

			// Create a new descriptor set layout handle for each of the recorded descriptor sets.
			for (const auto& mapping : m_descriptor_sets_mapping)
			{
				vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info;
				descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(mapping.second.size());
				descriptor_set_layout_create_info.pBindings = mapping.second.data();

				auto layout = m_device_ptr->get_handle().createDescriptorSetLayout(descriptor_set_layout_create_info);
				descriptor_set_layouts.push_back(layout);
			}

			return descriptor_set_layouts;
		}

		vk::DescriptorSetLayout DescriptorSetLayoutBuilder::build_layout_for_set(uint32_t set) const
		{
			if (m_is_recording)
			{
				throw std::runtime_error("The LayoutBuilder is still in a recording state - call `end_descriptor_set_record()` before `build_layouts()`.");
			}

			vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info;
			descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(m_descriptor_sets_mapping.at(set).size());
			descriptor_set_layout_create_info.pBindings = m_descriptor_sets_mapping.at(set).data();

			return m_device_ptr->get_handle().createDescriptorSetLayout(descriptor_set_layout_create_info);
		}

		std::map<vk::DescriptorType, uint32_t> DescriptorSetLayoutBuilder::get_descriptor_type_to_count_mapping() const
		{
			std::map<vk::DescriptorType, uint32_t> descriptor_type_to_count_mapping =
			{
				{ vk::DescriptorType::eCombinedImageSampler, 0 },
				{ vk::DescriptorType::eInputAttachment, 0 },
				{ vk::DescriptorType::eSampledImage, 0 },
				{ vk::DescriptorType::eSampler, 0 },
				{ vk::DescriptorType::eStorageBuffer, 0 },
				{ vk::DescriptorType::eStorageBufferDynamic, 0 },
				{ vk::DescriptorType::eStorageImage, 0 },
				{ vk::DescriptorType::eStorageTexelBuffer, 0 },
				{ vk::DescriptorType::eUniformBuffer, 0 },
				{ vk::DescriptorType::eUniformBufferDynamic, 0 },
				{ vk::DescriptorType::eUniformTexelBuffer, 0 }
			};

			// Iterate over all of the sets.
			for (const auto& mapping : m_descriptor_sets_mapping)
			{
				// Iterate over all of the bindings associated with this set, and increment the
				// appropriate entry in the map above.
				for (const auto& binding : mapping.second)
				{
					descriptor_type_to_count_mapping[binding.descriptorType]++;
				}
			}

			return descriptor_type_to_count_mapping;
		}

		std::map<vk::DescriptorType, uint32_t> DescriptorSetLayoutBuilder::get_descriptor_type_to_count_mapping_for_set(uint32_t set) const
		{
			std::map<vk::DescriptorType, uint32_t> descriptor_type_to_count_mapping =
			{
				{ vk::DescriptorType::eCombinedImageSampler, 0 },
				{ vk::DescriptorType::eInputAttachment, 0 },
				{ vk::DescriptorType::eSampledImage, 0 },
				{ vk::DescriptorType::eSampler, 0 },
				{ vk::DescriptorType::eStorageBuffer, 0 },
				{ vk::DescriptorType::eStorageBufferDynamic, 0 },
				{ vk::DescriptorType::eStorageImage, 0 },
				{ vk::DescriptorType::eStorageTexelBuffer, 0 },
				{ vk::DescriptorType::eUniformBuffer, 0 },
				{ vk::DescriptorType::eUniformBufferDynamic, 0 },
				{ vk::DescriptorType::eUniformTexelBuffer, 0 }
			};

			// Iterate over all of the bindings associated with this set, and increment the
			// appropriate entry in the map above.
			for (const auto& binding : m_descriptor_sets_mapping.at(set))
			{
				descriptor_type_to_count_mapping[binding.descriptorType]++;
			}

			return descriptor_type_to_count_mapping;
		}

		DescriptorPool::DescriptorPool(const Device& device, const std::vector<vk::DescriptorPoolSize>& descriptor_pool_sizes, uint32_t max_sets) :

			m_device_ptr(&device),
			m_descriptor_pool_sizes(descriptor_pool_sizes),
			m_max_sets(max_sets),
			m_available_sets(max_sets)
		{
			// Fill out the container that maps each descriptor type to the number of descriptors of that type
			// that can be allocated from this descriptor pool across ALL future descriptor sets. This will be 
			// used to track allocations from this pool.
			for (const auto& descriptor_pool_size : m_descriptor_pool_sizes)
			{
				auto type = descriptor_pool_size.type;
				auto count = descriptor_pool_size.descriptorCount;

				m_descriptor_type_to_count_available_mapping.at(type) += count;
			}

			vk::DescriptorPoolCreateInfo descriptor_pool_create_info;
			descriptor_pool_create_info.maxSets = max_sets;
			descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes.size());
			descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes.data();

			m_descriptor_pool_handle = m_device_ptr->get_handle().createDescriptorPoolUnique(descriptor_pool_create_info);
		}

	} // namespace graphics

} // namespace plume