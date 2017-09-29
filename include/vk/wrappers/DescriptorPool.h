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

#include "Device.h"

namespace plume
{

	namespace graphics
	{

		//! A simple class for creating and aggregating vk::DescriptorSetLayoutBinding structs, which are used
		//! to create a vk::DescriptorSetLayout. Each descriptor has a binding, which is a unique, numeric index 
		//! specified in the shader. Each binding is associated with a set. 
		//!
		//! Descriptor set entries are created by calling `begin_descriptor_set_record()` with the desired set 
		//! index. After a call to `begin_descriptor_set_record()`, all subsequent calls to `add_binding()`
		//! will add a binding to the current set. 
		//!
		//! Recording is terminated by calling `end_descriptor_set_record()`.
		//!
		//! Multiple sets can be recorded into a single DescriptorSetLayoutBuilder, and the handles to the 
		//! corresponding vk::DescriptorSetLayout objects can be retrieved by calling `build_layouts()` or 
		//! `build_layout_for_set()` if you wish to retrieve a vk::DescriptorSetLayout for a particular set.
		class DescriptorSetLayoutBuilder
		{
		public:

			//! Factory method for constructing a new shared DescriptorSetLayoutBuilder.
			static std::shared_ptr<DescriptorSetLayoutBuilder> create(const Device& device)
			{
				return std::shared_ptr<DescriptorSetLayoutBuilder>(new DescriptorSetLayoutBuilder(device));
			}

			//! Begin recording bindings into a new set.
			void begin_descriptor_set_record(uint32_t set)
			{
				m_current_set = set;
				m_is_recording = true;
			}

			//! End recording into the current set.
			void end_descriptor_set_record()
			{
				m_is_recording = false;
			}

			//! Adds a descriptor to the set at the current binding. By default, it is assumed that the descriptor
			//! is not an array (`count` is 1) and will be accessed by all shader stages in the pipeline. This 
			//! function must be called between `begin_descriptor_set_record()` and `end_descriptor_set_record()`.
			void add_binding(vk::DescriptorType type, uint32_t binding, uint32_t count = 1, vk::ShaderStageFlags stages = vk::ShaderStageFlagBits::eAll);

			/*
			 *
			 * Some useful shortcuts - all of these call `add_binding()` and simply fill out the first parameter.
			 *
			 */
			void add_ubo(uint32_t binding, uint32_t count = 1, vk::ShaderStageFlags stages = vk::ShaderStageFlagBits::eAll)
			{
				add_binding(vk::DescriptorType::eUniformBuffer, binding, count, stages);
			}

			void add_ubo_dynamic(uint32_t binding, uint32_t count = 1, vk::ShaderStageFlags stages = vk::ShaderStageFlagBits::eAll)
			{
				add_binding(vk::DescriptorType::eUniformBufferDynamic, binding, count, stages);
			}

			void add_ssbo(uint32_t binding, uint32_t count = 1, vk::ShaderStageFlags stages = vk::ShaderStageFlagBits::eAll)
			{
				add_binding(vk::DescriptorType::eStorageBuffer, binding, count, stages);
			}

			void add_ssbo_dynamic(uint32_t binding, uint32_t count = 1, vk::ShaderStageFlags stages = vk::ShaderStageFlagBits::eAll)
			{
				add_binding(vk::DescriptorType::eStorageBufferDynamic, binding, count, stages);
			}

			void add_tbo(uint32_t binding, uint32_t count = 1, vk::ShaderStageFlags stages = vk::ShaderStageFlagBits::eAll)
			{
				add_binding(vk::DescriptorType::eUniformTexelBuffer, binding, count, stages);
			}

			void add_cis(uint32_t binding, uint32_t count = 1, vk::ShaderStageFlags stages = vk::ShaderStageFlagBits::eAll)
			{
				add_binding(vk::DescriptorType::eCombinedImageSampler, binding, count, stages);
			}

			//! Uses all recorded descriptor sets and their associated layout bindings to create a vector of 
			//! descriptor set layouts. The resulting descriptor set layout handles will be in the order in which
			//! the descriptor sets were recorded into the DescriptorSetLayoutBuilder. For example, if you first record  
			//! bindings for set #1 then record bindings for set #0, the vk::DescriptorSetLayout at index 0 in the resulting
			//! vector will correspond to set #1. Similarly, the vk::DescriptorSetLayout at index 1 will correspond
			//! to set #0. To build a vk::DescriptorSetLayout for a particular set (or to enforce your own ordering),
			//! use `build_layout_for_set()`.
			std::vector<vk::DescriptorSetLayout> build_layouts() const;

			//! Creates a descriptor set layout for the set at the specified index `set`. To build descriptor set
			//! layouts for all sets simultaneously, see `build_layouts()`. Note that this function with throw an 
			//! exception if the set at the specified index does not exist.
			vk::DescriptorSetLayout build_layout_for_set(uint32_t set) const;

			//! Clears all previously recorded descriptor sets and descriptor set layout bindings.
			void reset()
			{
				m_descriptor_sets_mapping.clear();
				m_current_set = 0;
				m_is_recording = false;
			}

			//! Returns the number of descriptor sets that have been recorded.
			size_t get_num_sets() const
			{
				return m_descriptor_sets_mapping.size();
			}

			//! Returns the number of descriptor set layout bindings that have been recorded into the desciptor set
			//! at index `set`.
			size_t get_bindings_count_for_set(uint32_t set) const
			{
				return m_descriptor_sets_mapping.at(set).size();
			}

			//! Returns a vector of all descriptor set layout bindings that have been recorded into the descriptor 
			//! set at index `set`.
			const std::vector<vk::DescriptorSetLayoutBinding>& get_bindings_for_set(uint32_t set) const
			{
				return m_descriptor_sets_mapping.at(set);
			}

			//! Returns a map containing a key for each descriptor type that points to an integer value, where the 
			//! value represents the total number of descriptor set layout bindings that exist in this DescriptorSetLayoutBuilder 
			//! instance for that descriptor type across ALL recorded sets. Consider the following group of descriptor sets:
			//!
			//!				Set #0:
			//!					Binding #0: uniform buffer
			//!					Binding #1: uniform buffer
			//!					Binding #2: combined image sampler
			//!
			//!				Set #1:
			//!					Binding #0: storage buffer
			//!					Binding #1: input attachment
			//!
			//! In this scenario, `get_descriptor_type_count_mapping()` would return the following map:
			//!
			//!				{
			//!					{ vk::DescriptorType::eCombinedImageSampler, 1 },
			//!					{ vk::DescriptorType::eInputAttachment, 1 },
			//!					{ vk::DescriptorType::eStorageBuffer, 1 },
			//!					{ vk::DescriptorType::eUniformBuffer, 2 },
			//!					...
			//!				}
			//!
			//!	All other keys in the map would have a value of 0.
			std::map<vk::DescriptorType, uint32_t> get_descriptor_type_to_count_mapping() const;

			//! This function works like `get_descriptor_type_to_count_mapping()`, except it only returns
			//! entries for one particular set. See `get_descriptor_type_to_count_mapping()` for details.
			std::map<vk::DescriptorType, uint32_t> get_descriptor_type_to_count_mapping_for_set(uint32_t set) const;

			//! Returns the container that is used internally by this DescriptorSetLayoutBuilder to track
			//! associations between descriptor sets and descriptor set layout bindings. In particular, each
			//! key is the numeric index of a set. In a shader, this corresponds to:
			//!
			//!					layout (set = 0, binding = ...) ...
			//!							^^^^^^^
			//!
			//! Each value is a vector of bindings. In a shader, this corresponds to:
			//!
			//!					layout (set = ..., binding = 0) ...
			//!									   ^^^^^^^^^^^
			//! 
			//! Values are, in fact, vectors because each descriptor set can have many bindings.
			const std::map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>>& get_descriptor_sets_mapping() const
			{
				return m_descriptor_sets_mapping;
			}

		private:

			DescriptorSetLayoutBuilder(const Device& device) :

				m_device_ptr(&device),
				m_current_set(0),
				m_is_recording(false)
			{
			}

			const Device* m_device_ptr;

			uint32_t m_current_set;
			bool m_is_recording;
			std::map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>> m_descriptor_sets_mapping;

			friend class DescriptorPool;
		};

		//! Descriptor pools maintain a pool of descriptors, from which descriptor sets are allocated. They are 
		//! constructed by specifying one or more descriptor pool size structs, each of which contains a descriptor
		//! type (i.e. vk::DescriptorType::eUniformBuffer) and a descriptor count. The descriptor pool will 
		//! allocate enough storage for the total number of descriptors of each type.
		class DescriptorPool
		{
		public:

			//! Factory method for returning a new DescriptorPoolRef. Takes a list of descriptor pool size structs, each of
			//! which maps a descriptor type to a descriptor count. For example, a pool that is large enough to hold 3 uniform  
			//! buffers, 4 combined image samplers, and 1 storage buffer could be created like:
			//!			
			//!		auto pool = graphics::DescriptorPool::create(device, { { vk::DescriptorType::eUniformBuffer, 3 }, 
			//!															   { vk::DescriptorType::eCombinedImageSampler, 4},
			//!															   { vk::DescriptorType::eStorageBuffer, 1} });
			//!
			DescriptorPool(const Device& device, const std::vector<vk::DescriptorPoolSize>& descriptor_pool_sizes, uint32_t max_sets = 1);

			vk::DescriptorPool get_handle() const { return m_descriptor_pool_handle.get(); }

			//! Returns the maximum number of descriptor sets that can be safely allocated from this pool.
			uint32_t get_max_sets() const { return m_max_sets; }

			//! Returns the number of remaining descriptor sets that can be safely allocated from this pool. 
			//! This will always be less than or equal to the value returned by `get_max_sets()`.
			uint32_t get_available_sets() const { return m_available_sets; }

			//! Returns a container that maps each descriptor type to an integer that represents the remaining 
			//! number of descriptors of that type that can be safely allocated from this pool. If a particular
			//! key has a value of 0, then no more descriptors of this type can be allocated from the pool.
			const std::map<vk::DescriptorType, uint32_t>& get_descriptor_type_to_count_available_mapping() const
			{
				return m_descriptor_type_to_count_available_mapping;
			}

			//! Allocate one or more descriptor sets from the descriptor pool. The descriptor sets must have been previously 
			//! recorded into the DescriptorSetLayoutBuilder. The vector of `set_indices` is a list of integers that specify which 
			//! descriptor set layouts to allocate descriptor sets for. Each index should match one of the values that was passed  
			//! to the DescriptorSetLayoutBuilder's `begin_descriptor_set_record()` function.
			//!
			//! This allows you to allocate a subset of the descriptor sets that have been recorded into the DescriptorSetLayoutBuilder.
			//!
			//! The resulting vk::DescriptorSet handles will be returned in the order specified by `set_indices`. That is, if `set_indices`
			//! contains the values 1 and 0 (in that order), then element 0 of the vector returned by this function will be a vk::DescriptorSet 
			//! corresponding to the descriptor set layout that was recorded for set 1. Similarly, element 1 of the vector will be a 
			//! vk::DescriptorSet corresponding to the descriptor set layout that was recorded for set 0.
			std::vector<vk::DescriptorSet> allocate_descriptor_sets(const std::shared_ptr<DescriptorSetLayoutBuilder>& builder, const std::vector<uint32_t> set_indices)
			{
				// Verify that all of the requested sets actually exist in the DescriptorSetLayoutBuilder's map.
				for (auto set_index : set_indices)
				{
					if (builder->m_descriptor_sets_mapping.find(set_index) == builder->m_descriptor_sets_mapping.end())
					{
						throw std::runtime_error("One or more of the requested descriptor set indices was not found in the DescriptorSetLayoutBuilder's\
											      map of recorded descriptor sets");
					}
				}

				// Build a descriptor set layout object for each of the requested sets.
				std::vector<vk::DescriptorSetLayout> requested_layouts;
				for (auto set_index : set_indices)
				{
					requested_layouts.push_back(builder->build_layout_for_set(set_index));

					// Update this pool's internal mapping structure to reflect the new allocation(s). In other 
					// words, if this allocation requests 2 uniform buffers, subtract 2 from the current value
					// associated with vk::DescriptorType::eUniformBuffer in the pool's mapping.
					//
					// If this number goes below zero for any descriptor type, we need to throw an error. Also,
					// if the user has requested more sets than the `max_sets` parameter that was passed to the
					// DescriptorPool's constructor, we need to throw an error.
					auto descriptor_type_counts = builder->get_descriptor_type_to_count_mapping_for_set(set_index);
					for (const auto& mapping : descriptor_type_counts)
					{
						auto type = mapping.first;
						auto count = mapping.second;

						m_descriptor_type_to_count_available_mapping.at(type) -= count;
						if (m_descriptor_type_to_count_available_mapping.at(type) < 0)
						{
							throw std::runtime_error("One or more of the requested descriptor set allocations uses more resources than available in\
													  the DescriptorPool");
						}
					}

					m_available_sets--;

					if (m_available_sets < 0)
					{
						throw std::runtime_error("Attempting to allocate more descriptor sets than specified in the constructor for this DescriptorPool");
					}
				}

				vk::DescriptorSetAllocateInfo descriptor_set_allocate_info =
				{
					m_descriptor_pool_handle.get(),						// descriptor pool
					static_cast<uint32_t>(requested_layouts.size()),	// number of sets to allocate
					requested_layouts.data()							// descriptor set layout
				};

				// TODO: the descriptor set layouts created above need to either be destroyed or cached.

				// Allocate the descriptor sets.
				return m_device_ptr->get_handle().allocateDescriptorSets(descriptor_set_allocate_info);
			}

		public:

			const Device* m_device_ptr;
			vk::UniqueDescriptorPool m_descriptor_pool_handle;

			std::vector<vk::DescriptorPoolSize> m_descriptor_pool_sizes;
			uint32_t m_max_sets;
			uint32_t m_available_sets;

			std::map<vk::DescriptorType, uint32_t> m_descriptor_type_to_count_available_mapping =
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
		};

	} // namespace graphics

} // namespace plume