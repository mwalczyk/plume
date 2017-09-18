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
#include <map>
#include <vector>

#include "Platform.h"
#include "Noncopyable.h"
#include "Device.h"

namespace graphics
{

	class LayoutBuilder;
	using LayoutBuilderRef = std::shared_ptr<LayoutBuilder>;

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
	//! Multiple sets can be recorded into a single LayoutBuilder, and the handles to the corresponding 
	//! vk::DescriptorSetLayout objects can be retrieved by calling `build_layouts()` or `build_layout_for_set()`
	//! if you wish to retrieve a vk::DescriptorSetLayout for a particular set.
	class LayoutBuilder
	{
	public:

		//! Factory method for returning a new LayoutBuilderRef. 
		static LayoutBuilderRef create(DeviceWeakRef device)
		{
			return std::make_shared<LayoutBuilder>(device);
		}

		LayoutBuilder(DeviceWeakRef device) :
			m_device(device),
			m_current_set(0),
			m_is_recording(false)
		{
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
		//! is not an array (`count` is 1) and will be accessed by all shader stages in the pipeline. This function
		//! must be called between `begin_descriptor_set_record()` and `end_descriptor_set_record()`.
		void add_binding(vk::DescriptorType type, 
					     uint32_t binding = 0, 
						 uint32_t count = 1, 
						 vk::ShaderStageFlags stages = vk::ShaderStageFlagBits::eAll)
		{
			if (!m_is_recording)
			{
				throw std::runtime_error("Adding a new binding must be called between `begin_descriptor_set_record()` and\
										  `end_descriptor_set_record()`");
			}

			m_descriptor_sets_mapping[m_current_set].push_back({
				binding,		// binding (as it appears in the shader code)
				type,			// descriptor type (i.e. vk::DescriptorType::eUniformBuffer)
				count,			// descriptor count (if the descriptor is an array)
				stages,			// shader usage stages
				nullptr			// immutable samplers 
			});
		}

		//! Uses all recorded descriptor sets and their associated layout bindings to create a vector of 
		//! descriptor set layouts. The resulting descriptor set layout handles will be in the order in which
		//! the descriptor sets were recorded into the LayoutBuilder. For example, if you first record bindings 
		//! for set #1 then record bindings for set #0, the vk::DescriptorSetLayout at index 0 in the resulting
		//! vector will correspond to set #1. Similarly, the vk::DescriptorSetLayout at index 1 will correspond
		//! to set #0. To build a vk::DescriptorSetLayout for a particular set (or to enforce your own ordering),
		//! use `build_layout_for_set()`.
		std::vector<vk::DescriptorSetLayout> build_layouts() const
		{
			if (m_is_recording)
			{
				throw std::runtime_error("The LayoutBuilder is still in a recording state - call `end_descriptor_set_record()` before\
										  `build_layouts()`.");
			}

			DeviceRef device_shared = m_device.lock();

			std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;

			// Create a new descriptor set layout handle for each of the recorded descriptor sets.
			for (const auto& mapping : m_descriptor_sets_mapping)
			{
				vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info;
				descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(mapping.second.size());
				descriptor_set_layout_create_info.pBindings = mapping.second.data();

				auto layout = device_shared->get_handle().createDescriptorSetLayout(descriptor_set_layout_create_info);
				descriptor_set_layouts.push_back(layout);
			}

			return descriptor_set_layouts;
		}

		//! Creates a descriptor set layout for the set at the specified index `set`. To build descriptor set
		//! layouts for all sets simultaneously, see `build_layouts()`. Note that this function with throw an 
		//! exception if the set at the specified index does not exist.
		vk::DescriptorSetLayout build_layout_for_set(uint32_t set) const
		{
			if (m_is_recording)
			{
				throw std::runtime_error("The LayoutBuilder is still in a recording state - call `end_descriptor_set_record()` before\
										  `build_layouts()`.");
			}

			DeviceRef device_shared = m_device.lock();

			vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info;
			descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(m_descriptor_sets_mapping.at(set).size());
			descriptor_set_layout_create_info.pBindings = m_descriptor_sets_mapping.at(set).data();

			return device_shared->get_handle().createDescriptorSetLayout(descriptor_set_layout_create_info);
		}

		//! Clears all previously recorded descriptor sets and descriptor set layout bindings.
		void reset() 
		{ 
			m_descriptor_sets_mapping.clear(); 
			m_current_set = 0;
			m_is_recording = false;
		}

		//! Returns the number of descriptor sets that have been recorded.
		size_t get_num_sets() const { return m_descriptor_sets_mapping.size(); }

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
		//! value represents the number of descriptor set layout bindings that exist in this LayoutBuilder 
		//! instance for that descriptor type. Consider the following group of descriptor sets:
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
		std::map<vk::DescriptorType, uint32_t> get_descriptor_type_count_mapping() const
		{
			std::map<vk::DescriptorType, uint32_t> descriptor_type_count_mapping =
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
					descriptor_type_count_mapping[binding.descriptorType]++;
				}
			}

			return descriptor_type_count_mapping;
		}

	private:

		DeviceWeakRef m_device;
		uint32_t m_current_set;
		bool m_is_recording;
		std::map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>> m_descriptor_sets_mapping;
	};

	class DescriptorPool;
	using DescriptorPoolRef = std::shared_ptr<DescriptorPool>;

	//! Descriptor pools maintain a pool of descriptors, from which descriptor sets are allocated. They are 
	//! constructed by specifying one or more descriptor pool size structs, each of which contains a descriptor
	//! type (i.e. vk::DescriptorType::eUniformBuffer) and a descriptor count. The descriptor pool will 
	//! allocate enough storage for the total number of descriptors of each type.
	class DescriptorPool : public Noncopyable
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
		static DescriptorPoolRef create(DeviceWeakRef device, const std::vector<vk::DescriptorPoolSize>& descriptor_pool_sizes, uint32_t max_sets = 1)
		{

			return std::make_shared<DescriptorPool>(device, descriptor_pool_sizes, max_sets);
		}

		DescriptorPool(DeviceWeakRef device, const std::vector<vk::DescriptorPoolSize>& descriptor_pool_sizes, uint32_t max_sets = 1);

		~DescriptorPool();

		vk::DescriptorPool get_handle() const { return m_descriptor_pool_handle; }

		//! Allocate one or more descriptor sets from the descriptor pool. The descriptor sets must have been previously 
		//! recorded into the LayoutBuilder. The vector of `set_indices` is a list of integers that specify which descriptor
		//! set layouts to allocate descriptor sets with. Each index should match one of the values that was passed to 
		//! the LayoutBuilder's `begin_descriptor_set_record()` function.
		//!
		//! This allows you to allocate a subset of the descriptor sets that have been recorded into the LayoutBuilder.
		std::vector<vk::DescriptorSet> allocate_descriptor_sets(const LayoutBuilderRef& builder, const std::vector<uint32_t> set_indices)
		{
			// TODO: this function should ensure that the requested descriptor sets do not exceed the maximum
			// number of descriptors per type and maximum number of descriptor sets has been exceeded, based on the
			// parameters that were used to construct the descriptor pool.

			DeviceRef device_shared = m_device.lock();

			// Build a descriptor set layout object for each of the requested sets.
			std::vector<vk::DescriptorSetLayout> requested_layouts;
			for (auto set_index : set_indices)
			{
				requested_layouts.push_back(builder->build_layout_for_set(set_index));
			}

			vk::DescriptorSetAllocateInfo descriptor_set_allocate_info = 
			{
				m_descriptor_pool_handle,							// descriptor pool
				static_cast<uint32_t>(requested_layouts.size()),	// number of sets to allocate
				requested_layouts.data()							// descriptor set layout
			};

			// Allocate the descriptor sets.
			return device_shared->get_handle().allocateDescriptorSets(descriptor_set_allocate_info);
		}

	public:

		DeviceWeakRef m_device;
		vk::DescriptorPool m_descriptor_pool_handle;
	};

} // namespace graphics