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
	//! specified in the shader. The LayoutBuilder class creates vk::DescriptorSetLayoutBinding structs 
	//! starting at the index specified in the constructor. Each time the `add_next_binding()` method is 
	//! called, this index is incremented by one. 
	class LayoutBuilder
	{
	public:

		//! Factory method for returning a new LayoutBuilderRef. 
		static LayoutBuilderRef create(const DeviceRef& device, uint32_t start = 0)
		{
			return std::make_shared<LayoutBuilder>(device, start);
		}

		LayoutBuilder(const DeviceRef& device, uint32_t start = 0) :
			m_device(device),
			m_current_binding(start)
		{
		}
		
		void add_next_binding(vk::DescriptorType type, uint32_t count = 1, vk::ShaderStageFlags stages = vk::ShaderStageFlagBits::eAll)
		{
			m_layout_bindings.push_back({
				m_current_binding,								// binding (as it appears in the shader code)
				type,											// descriptor type (i.e. vk::DescriptorType::eUniformBuffer)
				count,											// descriptor count (if the descriptor is an array)
				stages,											// shader usage stages
				nullptr											// immutable samplers 
			});

			m_current_binding++;
		}

		vk::DescriptorSetLayout build_layout()
		{
			vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info;
			descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(m_layout_bindings.size());
			descriptor_set_layout_create_info.pBindings = m_layout_bindings.data();

			return m_device->get_handle().createDescriptorSetLayout(descriptor_set_layout_create_info);
		}

		inline void skip(uint32_t skip_count) { m_current_binding += skip_count; }
		inline void reset() { m_current_binding = 0; m_layout_bindings.clear(); }
		inline void reset_and_start_at(uint32_t start) { m_current_binding = start; m_layout_bindings.clear(); }
		inline size_t get_bindings_count() const { return m_layout_bindings.size(); }
		inline const std::vector<vk::DescriptorSetLayoutBinding>& get_bindings() const { return m_layout_bindings; }

	private:

		DeviceRef m_device;
		uint32_t m_current_binding;
		std::vector<vk::DescriptorSetLayoutBinding> m_layout_bindings;
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
		static DescriptorPoolRef create(const DeviceRef& device, const std::vector<vk::DescriptorPoolSize>& descriptor_pool_sizes, uint32_t max_sets = 1)
		{

			return std::make_shared<DescriptorPool>(device, descriptor_pool_sizes, max_sets);
		}

		DescriptorPool(const DeviceRef& device, const std::vector<vk::DescriptorPoolSize>& descriptor_pool_sizes, uint32_t max_sets = 1);

		inline vk::DescriptorPool get_handle() const { return m_descriptor_pool_handle; }

	public:

		DeviceRef m_device;
		vk::DescriptorPool m_descriptor_pool_handle;
	};

} // namespace graphics