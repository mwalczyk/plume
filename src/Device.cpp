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

#include "Device.h"

namespace graphics
{

	Device::Options::Options()
	{
		m_required_queue_flags = vk::QueueFlagBits::eGraphics;
		m_use_swapchain = true;
	}

	Device::Device(vk::PhysicalDevice physical_device, const Options& options) :
		m_physical_device_handle(physical_device),
		m_required_device_extensions(options.m_required_device_extensions)
	{		
		// Store the general properties, features, and memory properties of the chosen physical device.
		m_physical_device_properties = m_physical_device_handle.getProperties();
		m_physical_device_features = m_physical_device_handle.getFeatures();
		m_physical_device_memory_properties = m_physical_device_handle.getMemoryProperties();
		
		// Store the queue family properties of the chosen physical device.
		m_physical_device_queue_family_properties = m_physical_device_handle.getQueueFamilyProperties();

		// Store the device extensions of the chosen physical device.
		m_physical_device_extension_properties = m_physical_device_handle.enumerateDeviceExtensionProperties();
		
		// Find the indicies of all of the requested queue families (inspired by Sascha Willems' codebase).
		const float default_queue_priority = 0.0f;
		const uint32_t default_queue_count = 1;
		std::vector<vk::DeviceQueueCreateInfo> device_queue_create_infos;
		if (options.m_required_queue_flags & vk::QueueFlagBits::eGraphics)
		{
			m_queue_families_mapping.m_graphics_queue.second = find_queue_family_index(vk::QueueFlagBits::eGraphics);

			vk::DeviceQueueCreateInfo device_queue_create_info = {};
			device_queue_create_info.pQueuePriorities = &default_queue_priority;
			device_queue_create_info.queueCount = default_queue_count;
			device_queue_create_info.queueFamilyIndex = m_queue_families_mapping.m_graphics_queue.second;

			device_queue_create_infos.push_back(device_queue_create_info);

			// For now, perform presentation with the same queue as graphics operations.
			if (options.m_use_swapchain)
			{
				m_queue_families_mapping.m_presentation_queue.second = m_queue_families_mapping.m_graphics_queue.second;
			}
		}
		if (options.m_required_queue_flags & vk::QueueFlagBits::eCompute)
		{
			m_queue_families_mapping.m_compute_queue.second = find_queue_family_index(vk::QueueFlagBits::eCompute);

			if (m_queue_families_mapping.m_compute_queue.second != m_queue_families_mapping.m_graphics_queue.second)
			{
				// Create a dedicated queue for compute operations.
				vk::DeviceQueueCreateInfo device_queue_create_info = {};
				device_queue_create_info.pQueuePriorities = &default_queue_priority;
				device_queue_create_info.queueCount = default_queue_count;
				device_queue_create_info.queueFamilyIndex = m_queue_families_mapping.m_compute_queue.second;

				device_queue_create_infos.push_back(device_queue_create_info);
			}
			else
			{
				// Reuse the graphics queue for compute operations.
				m_queue_families_mapping.m_compute_queue.second = m_queue_families_mapping.m_graphics_queue.second;
			}
		}
		if (options.m_required_queue_flags & vk::QueueFlagBits::eTransfer)
		{
			m_queue_families_mapping.m_transfer_queue.second = find_queue_family_index(vk::QueueFlagBits::eTransfer);

			if (m_queue_families_mapping.m_transfer_queue.second != m_queue_families_mapping.m_graphics_queue.second &&
				m_queue_families_mapping.m_transfer_queue.second != m_queue_families_mapping.m_compute_queue.second)
			{
				// Create a dedicated queue for transfer operations.
				vk::DeviceQueueCreateInfo device_queue_create_info = {};
				device_queue_create_info.pQueuePriorities = &default_queue_priority;
				device_queue_create_info.queueCount = default_queue_count;
				device_queue_create_info.queueFamilyIndex = m_queue_families_mapping.m_transfer_queue.second;

				device_queue_create_infos.push_back(device_queue_create_info);
			}
			else
			{
				// Reuse the graphics queue for transfer operations.
				m_queue_families_mapping.m_transfer_queue.second = m_queue_families_mapping.m_graphics_queue.second;
			}
		}
		if (options.m_required_queue_flags & vk::QueueFlagBits::eSparseBinding)
		{
			m_queue_families_mapping.m_sparse_binding_queue.second = find_queue_family_index(vk::QueueFlagBits::eSparseBinding);

			// TODO
		}

		// Automatically add the swapchain extension if needed.
		if (options.m_use_swapchain)
		{
			m_required_device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		// Create the logical device: note that device layers were deprecated in Vulkan 1.0.13, and device layer 
		// requests should be ignored by the driver.
		vk::DeviceCreateInfo device_create_info;
		device_create_info.enabledExtensionCount = static_cast<uint32_t>(m_required_device_extensions.size());
		device_create_info.pEnabledFeatures = &m_physical_device_features;
		device_create_info.ppEnabledExtensionNames = m_required_device_extensions.data();
		device_create_info.pQueueCreateInfos = device_queue_create_infos.data();
		device_create_info.queueCreateInfoCount = static_cast<uint32_t>(device_queue_create_infos.size());

		m_device_handle = m_physical_device_handle.createDevice(device_create_info);

		// Store handles to each of the newly created queues.
		m_queue_families_mapping.m_graphics_queue.first = m_device_handle.getQueue(m_queue_families_mapping.m_graphics_queue.second, 0);
		m_queue_families_mapping.m_compute_queue.first = m_device_handle.getQueue(m_queue_families_mapping.m_compute_queue.second, 0);
		m_queue_families_mapping.m_transfer_queue.first = m_device_handle.getQueue(m_queue_families_mapping.m_transfer_queue.second, 0);
		m_queue_families_mapping.m_sparse_binding_queue.first = m_device_handle.getQueue(m_queue_families_mapping.m_sparse_binding_queue.second, 0);
		m_queue_families_mapping.m_presentation_queue.first = m_device_handle.getQueue(m_queue_families_mapping.m_presentation_queue.second, 0);
	}

	Device::~Device()
	{
		// The logical device is likely to be the last object created (aside from objects used at
		// runtime). Before destroying the device, ensure that it is not executing any work.
		m_device_handle.waitIdle();
		
		// Note that queues are created along with the logical device. All queues associated with 
		// this device will automatically be destroyed when vkDestroyDevice is called.
		m_device_handle.destroy();
	}

	uint32_t Device::find_queue_family_index(vk::QueueFlagBits queue_flag_bits) const
	{
		// Try to find a dedicated queue for compute operations (without graphics).
		if (queue_flag_bits == vk::QueueFlagBits::eCompute)
		{
			for (size_t i = 0; i < m_physical_device_queue_family_properties.size(); ++i)
			{
				if (m_physical_device_queue_family_properties[i].queueCount > 0 &&
					m_physical_device_queue_family_properties[i].queueFlags & queue_flag_bits &&
					(m_physical_device_queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlagBits::eGraphics)
				{
					return static_cast<uint32_t>(i);
				}
			}
		}
		// Try to find a dedicated queue for transfer operations (without compute and graphics).
		else if (queue_flag_bits == vk::QueueFlagBits::eTransfer)
		{
			for (size_t i = 0; i < m_physical_device_queue_family_properties.size(); ++i)
			{
				if (m_physical_device_queue_family_properties[i].queueCount > 0 &&
					m_physical_device_queue_family_properties[i].queueFlags & queue_flag_bits &&
					(m_physical_device_queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlagBits::eGraphics &&
					(m_physical_device_queue_family_properties[i].queueFlags & vk::QueueFlagBits::eCompute) != vk::QueueFlagBits::eCompute)
				{
					return static_cast<uint32_t>(i);
				}
			}
		}
		
		// For all other queue families (or if a dedicated queue was not found above), simply return the 
		// index of the first queue family that supports the requested operations.
		for (size_t i = 0; i < m_physical_device_queue_family_properties.size(); ++i)
		{
			if (m_physical_device_queue_family_properties[i].queueCount > 0 && m_physical_device_queue_family_properties[i].queueFlags & queue_flag_bits)
			{
				return static_cast<uint32_t>(i);
			}
		}
		
		throw std::runtime_error("Could not find a matching queue family");
	}

	vk::Format Device::get_supported_depth_format() const
	{
		static const std::vector<vk::Format> depth_formats = 
		{
			vk::Format::eD32SfloatS8Uint,
			vk::Format::eD32Sfloat,
			vk::Format::eD24UnormS8Uint,
			vk::Format::eD16UnormS8Uint,
			vk::Format::eD16Unorm
		};

		for (const auto& format : depth_formats)
		{
			auto format_properties = get_physical_device_format_properties(format);
			if (format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
			{
				return format;
			}
		}
		
		return vk::Format::eUndefined;
	}

	Device::SwapchainSupportDetails Device::get_swapchain_support_details(const SurfaceRef& surface) const
	{
		SwapchainSupportDetails support_details;

		// Return the basic surface capabilities, i.e. min/max number of images, min/max width and height, etc.
		support_details.m_capabilities = m_physical_device_handle.getSurfaceCapabilitiesKHR(surface->get_handle());

		// Retrieve the available surface formats, i.e. pixel formats and color spaces.
		support_details.m_formats = m_physical_device_handle.getSurfaceFormatsKHR(surface->get_handle());

		// Retrieve the surface presentation modes, i.e. VK_PRESENT_MODE_MAILBOX_KHR.
		support_details.m_present_modes = m_physical_device_handle.getSurfacePresentModesKHR(surface->get_handle());

		if (support_details.m_formats.size() == 0 || support_details.m_present_modes.size() == 0)
		{
			throw std::runtime_error("No available surface formats or present modes found");
		}

		return support_details;
	}

	std::ostream& operator<<(std::ostream& stream, const DeviceRef& device)
	{
		stream << "Device object: " << device->m_device_handle << std::endl;
		
		stream << "Chosen physical device object: " << device->m_physical_device_handle << std::endl;
		std::cout << "\tDevice ID: " << device->m_physical_device_properties.deviceID << std::endl;
		std::cout << "\tDevice name: " << device->m_physical_device_properties.deviceName << std::endl;
		std::cout << "\tVendor ID: " << device->m_physical_device_properties.vendorID << std::endl;

		stream << "Queue family details:" << std::endl;
		stream << "\tQueue family - graphics index: " << device->m_queue_families_mapping.graphics().second << std::endl;
		stream << "\tQueue family - compute index: " << device->m_queue_families_mapping.compute().second << std::endl;
		stream << "\tQueue family - transfer index: " << device->m_queue_families_mapping.transfer().second << std::endl;
		stream << "\tQueue family - sparse binding index: " << device->m_queue_families_mapping.sparse_binding().second << std::endl;
		stream << "\tQueue family - present index: " << device->m_queue_families_mapping.presentation().second << std::endl;

		return stream;
	}

} // namespace graphics