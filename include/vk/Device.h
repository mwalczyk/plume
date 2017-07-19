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

	class Device : public Noncopyable
	{
	public:

		struct SwapchainSupportDetails
		{
			vk::SurfaceCapabilitiesKHR m_capabilities;
			std::vector<vk::SurfaceFormatKHR> m_formats;
			std::vector<vk::PresentModeKHR> m_present_modes;
		};

		class QueueFamiliesMapping
		{
		public:
			QueueFamiliesMapping() = default;		

			std::pair<vk::Queue, uint32_t> graphics() const { return m_graphics_queue; }
			std::pair<vk::Queue, uint32_t> compute() const { return m_compute_queue; }
			std::pair<vk::Queue, uint32_t> transfer() const { return m_transfer_queue; }
			std::pair<vk::Queue, uint32_t> sparse_binding() const { return m_sparse_binding_queue; }
			std::pair<vk::Queue, uint32_t> presentation() const { return m_presentation_queue; }

		private:
			
			std::pair<vk::Queue, uint32_t> m_graphics_queue = { {}, 0 };
			std::pair<vk::Queue, uint32_t> m_compute_queue = { {}, 0 };
			std::pair<vk::Queue, uint32_t> m_transfer_queue = { {}, 0 };
			std::pair<vk::Queue, uint32_t> m_sparse_binding_queue = { {}, 0 };
			std::pair<vk::Queue, uint32_t> m_presentation_queue = { {}, 0 };

			friend class Device;
		};

		class Options
		{
		public:

			Options();

			Options& required_queue_flags(vk::QueueFlags required_queue_flags) { m_required_queue_flags = required_queue_flags; return *this; }
			Options& required_device_extensions(const std::vector<const char*>& required_device_extensions) { m_required_device_extensions = required_device_extensions; return *this; }
			Options& use_swapchain(bool use_swapchain) { m_use_swapchain = use_swapchain; return *this; }

		private:

			vk::QueueFlags m_required_queue_flags;
			std::vector<const char*> m_required_device_extensions;
			bool m_use_swapchain;

			friend class Device;
		};

		//! Factory method for returning a new DeviceRef.
		static DeviceRef create(vk::PhysicalDevice physical_device, const Options& options = Options())
		{
			return std::make_shared<Device>(physical_device, options);
		}

		//! Construct a logical device around a physical device (GPU).
		Device(vk::PhysicalDevice physical_device, const Options& options = Options());
		~Device();

		inline vk::Device get_handle() const { return m_device_handle; };
		inline vk::PhysicalDevice get_physical_device_handle() const { return m_physical_device_handle; }
		inline vk::PhysicalDeviceProperties get_physical_device_properties() const { return m_physical_device_properties; }
		inline vk::PhysicalDeviceFeatures get_physical_device_features() const { return m_physical_device_features;  }
		inline vk::PhysicalDeviceMemoryProperties get_physical_device_memory_properties() const { return m_physical_device_memory_properties; }
		inline const QueueFamiliesMapping& get_queue_families_mapping() const { return m_queue_families_mapping; }
		inline const std::vector<vk::QueueFamilyProperties>& get_physical_device_queue_family_properties() const { return m_physical_device_queue_family_properties; }
		inline const std::vector<vk::ExtensionProperties>& get_physical_device_extension_properties() const { return m_physical_device_extension_properties; }
		
		//! Format features are properties of the physical device.
		inline vk::FormatProperties get_physical_device_format_properties(vk::Format format) const { return m_physical_device_handle.getFormatProperties(format); }
		
		//! Depth formats are not necessarily supported by the system. Retrieve the highest precision format available.
		vk::Format get_supported_depth_format() const;

		//! Returns a structure that contains information related to the chosen physical device's swapchain support.
		SwapchainSupportDetails get_swapchain_support_details(const SurfaceRef& surface) const;

		friend std::ostream& operator<<(std::ostream& stream, const DeviceRef& device);

	private:

		uint32_t find_queue_family_index(vk::QueueFlagBits queue_flag_bits) const;

		vk::Device m_device_handle;
		vk::PhysicalDevice m_physical_device_handle;
		vk::PhysicalDeviceProperties m_physical_device_properties;
		vk::PhysicalDeviceFeatures m_physical_device_features;
		vk::PhysicalDeviceMemoryProperties m_physical_device_memory_properties;
		std::vector<vk::QueueFamilyProperties> m_physical_device_queue_family_properties;
		std::vector<vk::ExtensionProperties> m_physical_device_extension_properties;
		QueueFamiliesMapping m_queue_families_mapping;
		std::vector<const char*> m_required_device_extensions;
	};

} // namespace graphics