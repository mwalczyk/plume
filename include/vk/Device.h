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
#include <cassert>

#include "Platform.h"
#include "Noncopyable.h"
#include "Instance.h"
#include "Surface.h"

namespace graphics
{
	class Device;
	using DeviceRef = std::shared_ptr<Device>;

	// TODO: forward declaring this class for the `one_time_submit()` function causes a whole bunch of errors...
	//class CommandBuffer;
	//using CommandBufferRef = std::shared_ptr<CommandBuffer>;

	class Device : public Noncopyable
	{
	public:

		struct SwapchainSupportDetails
		{
			vk::SurfaceCapabilitiesKHR m_capabilities;
			std::vector<vk::SurfaceFormatKHR> m_formats;
			std::vector<vk::PresentModeKHR> m_present_modes;
		};

		enum class QueueType
		{
			GRAPHICS,
			COMPUTE,
			TRANSFER,
			SPARSE_BINDING,
			PRESENTATION
		};

		struct QueueInternals
		{
			uint32_t index = 0;
			vk::Queue handle = {};
		};

		//! Factory method for returning a new DeviceRef.
		static DeviceRef create(vk::PhysicalDevice physical_device,
			vk::QueueFlags required_queue_flags = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer,
			bool use_swapchain = true,
			const std::vector<const char*>& required_device_extensions = {})
		{
			return std::make_shared<Device>(physical_device, required_queue_flags, use_swapchain, required_device_extensions);
		}

		//! Construct a logical device around a physical device (GPU).
		Device(vk::PhysicalDevice physical_device, 
			vk::QueueFlags required_queue_flags = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer,
			bool use_swapchain = true, 
			const std::vector<const char*>& required_device_extensions = {});

		~Device();

		inline vk::Device get_handle() const { return m_device_handle; };
		inline vk::PhysicalDevice get_physical_device_handle() const { return m_physical_device_handle; }
		
		inline vk::PhysicalDeviceProperties get_physical_device_properties() const { return m_physical_device_properties; }
		inline vk::PhysicalDeviceFeatures get_physical_device_features() const { return m_physical_device_features;  }
		inline vk::PhysicalDeviceMemoryProperties get_physical_device_memory_properties() const { return m_physical_device_memory_properties; }
		
		inline const std::map<QueueType, QueueInternals>& get_queue_families_mapping() const { return m_queue_families_mapping; }
		inline uint32_t get_queue_family_index(QueueType type) { return m_queue_families_mapping[type].index; }
		inline vk::Queue get_queue_handle(QueueType type) { return m_queue_families_mapping[type].handle; }

		void one_time_submit(QueueType type, vk::CommandBuffer command_buffer_handle)
		{
			//vk::CommandBuffer command_buffer_handle = command_buffer->get_handle();

			vk::SubmitInfo submit_info;
			submit_info.commandBufferCount = 1;
			submit_info.pCommandBuffers = &command_buffer_handle;

			get_queue_handle(type).submit(submit_info, {});
			get_queue_handle(type).waitIdle();
		}

		inline const std::vector<vk::QueueFamilyProperties>& get_physical_device_queue_family_properties() const { return m_physical_device_queue_family_properties; }
		inline const std::vector<vk::ExtensionProperties>& get_physical_device_extension_properties() const { return m_physical_device_extension_properties; }
		
		//! Wait for all commands submitted to all queues to finish.
		inline void wait_idle() { m_device_handle.waitIdle(); }

		//! Format features are properties of the physical device.
		inline vk::FormatProperties get_physical_device_format_properties(vk::Format format) const { return m_physical_device_handle.getFormatProperties(format); }
		
		//! Depth formats are not necessarily supported by the system. Retrieve the highest precision format available.
		vk::Format get_supported_depth_format() const;

		//! Returns a structure that contains information related to the chosen physical device's swapchain support.
		SwapchainSupportDetails get_swapchain_support_details(const SurfaceRef& surface) const;

		friend std::ostream& operator<<(std::ostream& stream, const DeviceRef& device);

	private:

		std::map<QueueType, QueueInternals> m_queue_families_mapping = 
		{
			{ QueueType::GRAPHICS, {} },
			{ QueueType::COMPUTE, {} },
			{ QueueType::TRANSFER, {} },
			{ QueueType::SPARSE_BINDING, {} },
			{ QueueType::PRESENTATION, {} }
		};

		uint32_t find_queue_family_index(vk::QueueFlagBits queue_flag_bits) const;

		vk::Device m_device_handle;
		vk::PhysicalDevice m_physical_device_handle;
		vk::PhysicalDeviceProperties m_physical_device_properties;
		vk::PhysicalDeviceFeatures m_physical_device_features;
		vk::PhysicalDeviceMemoryProperties m_physical_device_memory_properties;
		std::vector<vk::QueueFamilyProperties> m_physical_device_queue_family_properties;
		std::vector<vk::ExtensionProperties> m_physical_device_extension_properties;
		std::vector<const char*> m_required_device_extensions;
	};

} // namespace graphics