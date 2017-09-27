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
#include "CommandBuffer.h"
#include "Semaphore.h"
#include "Swapchain.h"

namespace plume
{

	namespace graphics
	{
		Device::Device(vk::PhysicalDevice physical_device, 
					   const vk::UniqueSurfaceKHR& surface, 
					   vk::QueueFlags required_queue_flags, 
					   bool use_swapchain, 
					   const std::vector<const char*>& required_device_extensions) :

			m_required_device_extensions(required_device_extensions)
		{
			// Store the general properties, features, and memory properties of the chosen physical device.
			m_gpu_details =
			{
				physical_device,
				physical_device.getProperties(),
				physical_device.getFeatures(),
				physical_device.getMemoryProperties(),
				physical_device.getQueueFamilyProperties(),
				physical_device.enumerateDeviceExtensionProperties()
			};

			// Find the indicies of all of the requested queue families (inspired by Sascha Willems' codebase).
			const float default_queue_priority = 0.0f;
			const uint32_t default_queue_count = 1;
			std::vector<vk::DeviceQueueCreateInfo> device_queue_create_infos;
			if (required_queue_flags & vk::QueueFlagBits::eGraphics)
			{
				m_queue_families_mapping[QueueType::GRAPHICS].index = find_queue_family_index(vk::QueueFlagBits::eGraphics);

				vk::DeviceQueueCreateInfo device_queue_create_info = {};
				device_queue_create_info.pQueuePriorities = &default_queue_priority;
				device_queue_create_info.queueCount = default_queue_count;
				device_queue_create_info.queueFamilyIndex = m_queue_families_mapping[QueueType::GRAPHICS].index;

				device_queue_create_infos.push_back(device_queue_create_info);

				// For now, perform presentation with the same queue as graphics operations.
				if (use_swapchain)
				{
					m_queue_families_mapping[QueueType::PRESENTATION].index = m_queue_families_mapping[QueueType::GRAPHICS].index;
				}
			}
			if (required_queue_flags & vk::QueueFlagBits::eCompute)
			{
				m_queue_families_mapping[QueueType::COMPUTE].index = find_queue_family_index(vk::QueueFlagBits::eCompute);

				if (m_queue_families_mapping[QueueType::COMPUTE].index != m_queue_families_mapping[QueueType::GRAPHICS].index)
				{
					// Create a dedicated queue for compute operations.
					vk::DeviceQueueCreateInfo device_queue_create_info = {};
					device_queue_create_info.pQueuePriorities = &default_queue_priority;
					device_queue_create_info.queueCount = default_queue_count;
					device_queue_create_info.queueFamilyIndex = m_queue_families_mapping[QueueType::COMPUTE].index;

					device_queue_create_infos.push_back(device_queue_create_info);
				}
				else
				{
					// Reuse the graphics queue for compute operations.
					m_queue_families_mapping[QueueType::COMPUTE].index = m_queue_families_mapping[QueueType::GRAPHICS].index;
				}
			}
			if (required_queue_flags & vk::QueueFlagBits::eTransfer)
			{
				m_queue_families_mapping[QueueType::TRANSFER].index = find_queue_family_index(vk::QueueFlagBits::eTransfer);

				if (m_queue_families_mapping[QueueType::TRANSFER].index != m_queue_families_mapping[QueueType::GRAPHICS].index &&
					m_queue_families_mapping[QueueType::TRANSFER].index != m_queue_families_mapping[QueueType::COMPUTE].index)
				{
					// Create a dedicated queue for transfer operations.
					vk::DeviceQueueCreateInfo device_queue_create_info = {};
					device_queue_create_info.pQueuePriorities = &default_queue_priority;
					device_queue_create_info.queueCount = default_queue_count;
					device_queue_create_info.queueFamilyIndex = m_queue_families_mapping[QueueType::TRANSFER].index;

					device_queue_create_infos.push_back(device_queue_create_info);
				}
				else
				{
					// Reuse the graphics queue for transfer operations.
					m_queue_families_mapping[QueueType::TRANSFER].index = m_queue_families_mapping[QueueType::GRAPHICS].index;
				}
			}
			if (required_queue_flags & vk::QueueFlagBits::eSparseBinding)
			{
				m_queue_families_mapping[QueueType::SPARSE_BINDING].index = find_queue_family_index(vk::QueueFlagBits::eSparseBinding);

				// TODO: create a queue for sparse bindings.
			}

			// Automatically add the swapchain extension if needed.
			if (use_swapchain)
			{
				m_required_device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

				// Make sure that at least one queue family from the supplied physical device actually 
				// supports presentation with respect to the requested surface.
				for (size_t i = 0; i < m_gpu_details.m_queue_family_properties.size(); ++i)
				{
					vk::Bool32 support = m_gpu_details.m_handle.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface.get());
					if (!support)
					{
						break;
						// TODO: shouldn't this throw an error?
						//throw std::runtime_error("This physical device does not support presentation to the requested surface");
					}
				}
			}

			// TODO: should we do this instead, so as to not avoid the overhead of enabling every physical
			// device feature?
			/*vk::PhysicalDeviceFeatures enabled_features;
			if (m_gpu_details.m_features.tessellationShader)
			{
				enabled_features.tessellationShader = VK_TRUE;
			}
			if (m_gpu_details.m_features.geometryShader)
			{
				enabled_features.geometryShader = VK_TRUE;
			}*/

			// Create the logical device: note that device layers were deprecated, and device layer 
			// requests should be ignored by the driver. 
			// See: https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#extended-functionality-device-layer-deprecation
			vk::DeviceCreateInfo device_create_info;
			device_create_info.enabledExtensionCount = static_cast<uint32_t>(m_required_device_extensions.size());
			device_create_info.pEnabledFeatures = &m_gpu_details.m_features;
			device_create_info.ppEnabledExtensionNames = m_required_device_extensions.data();
			device_create_info.pQueueCreateInfos = device_queue_create_infos.data();
			device_create_info.queueCreateInfoCount = static_cast<uint32_t>(device_queue_create_infos.size());

			m_device_handle = m_gpu_details.m_handle.createDeviceUnique(device_create_info);

			// Store handles to each of the newly created queues.
			m_queue_families_mapping[QueueType::GRAPHICS].handle = m_device_handle->getQueue(m_queue_families_mapping[QueueType::GRAPHICS].index, 0);
			m_queue_families_mapping[QueueType::COMPUTE].handle = m_device_handle->getQueue(m_queue_families_mapping[QueueType::COMPUTE].index, 0);
			m_queue_families_mapping[QueueType::TRANSFER].handle = m_device_handle->getQueue(m_queue_families_mapping[QueueType::TRANSFER].index, 0);
			m_queue_families_mapping[QueueType::SPARSE_BINDING].handle = m_device_handle->getQueue(m_queue_families_mapping[QueueType::SPARSE_BINDING].index, 0);
			m_queue_families_mapping[QueueType::PRESENTATION].handle = m_device_handle->getQueue(m_queue_families_mapping[QueueType::PRESENTATION].index, 0);
		}

		Device::~Device()
		{
			// The logical device is likely to be the last object created (aside from objects used at
			// runtime). Before destroying the device, ensure that it is not executing any work.
			//
			// Note that queues are created along with the logical device. All queues associated with 
			// this device will automatically be destroyed when vkDestroyDevice is called.
			wait_idle();
		}

		uint32_t Device::find_queue_family_index(vk::QueueFlagBits queue_flag_bits) const
		{
			// Try to find a dedicated queue for compute operations (without graphics).
			if (queue_flag_bits == vk::QueueFlagBits::eCompute)
			{
				for (size_t i = 0; i < m_gpu_details.m_queue_family_properties.size(); ++i)
				{
					if (m_gpu_details.m_queue_family_properties[i].queueCount > 0 &&
						m_gpu_details.m_queue_family_properties[i].queueFlags & queue_flag_bits &&
						(m_gpu_details.m_queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlagBits::eGraphics)
					{
						return static_cast<uint32_t>(i);
					}
				}
			}
			// Try to find a dedicated queue for transfer operations (without compute and graphics).
			else if (queue_flag_bits == vk::QueueFlagBits::eTransfer)
			{
				for (size_t i = 0; i < m_gpu_details.m_queue_family_properties.size(); ++i)
				{
					if (m_gpu_details.m_queue_family_properties[i].queueCount > 0 &&
						m_gpu_details.m_queue_family_properties[i].queueFlags & queue_flag_bits &&
						(m_gpu_details.m_queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) != vk::QueueFlagBits::eGraphics &&
						(m_gpu_details.m_queue_family_properties[i].queueFlags & vk::QueueFlagBits::eCompute) != vk::QueueFlagBits::eCompute)
					{
						return static_cast<uint32_t>(i);
					}
				}
			}

			// For all other queue families (or if a dedicated queue was not found above), simply return the 
			// index of the first queue family that supports the requested operations.
			for (size_t i = 0; i < m_gpu_details.m_queue_family_properties.size(); ++i)
			{
				if (m_gpu_details.m_queue_family_properties[i].queueCount > 0 && m_gpu_details.m_queue_family_properties[i].queueFlags & queue_flag_bits)
				{
					return static_cast<uint32_t>(i);
				}
			}

			throw std::runtime_error("Could not find a matching queue family");
		}

		Device::SwapchainSupportDetails Device::get_swapchain_support_details(const vk::UniqueSurfaceKHR& surface) const
		{
			SwapchainSupportDetails support_details;

			// Return the basic surface capabilities, i.e. min/max number of images, min/max width and height, etc.
			support_details.m_capabilities = m_gpu_details.m_handle.getSurfaceCapabilitiesKHR(surface.get());

			// Retrieve the available surface formats, i.e. pixel formats and color spaces.
			support_details.m_formats = m_gpu_details.m_handle.getSurfaceFormatsKHR(surface.get());

			// Retrieve the surface presentation modes, i.e. VK_PRESENT_MODE_MAILBOX_KHR.
			support_details.m_present_modes = m_gpu_details.m_handle.getSurfacePresentModesKHR(surface.get());

			if (support_details.m_formats.size() == 0 || support_details.m_present_modes.size() == 0)
			{
				throw std::runtime_error("No available surface formats or present modes found");
			}

			return support_details;
		}

		uint32_t Device::acquire_next_swapchain_image(const Swapchain& swapchain, const Semaphore& semaphore, uint32_t timeout)
		{
			auto result = m_device_handle.get().acquireNextImageKHR(swapchain.get_handle(), timeout, semaphore.get_handle(), {});
			return result.value;
		}

		void Device::submit_with_semaphores(QueueType type,
											const CommandBuffer& command_buffer,
											const Semaphore& wait,
											const Semaphore& signal,
											vk::PipelineStageFlags pipeline_stage_flags)
		{
			vk::CommandBuffer command_buffer_handle = command_buffer.get_handle();

			vk::SubmitInfo submit_info = {};
			submit_info.waitSemaphoreCount = 1;
			submit_info.pWaitSemaphores = &wait.get_handle();
			submit_info.pWaitDstStageMask = &pipeline_stage_flags;
			submit_info.commandBufferCount = 1;
			submit_info.pCommandBuffers = &command_buffer_handle;
			submit_info.signalSemaphoreCount = 1;
			submit_info.pSignalSemaphores = &signal.get_handle();

			// TODO: this should support fences.
			get_queue_handle(type).submit(submit_info, {});
		}

		void one_time_submit(QueueType type, std::function<void(const CommandBuffer&)> func)
		{
			// TODO: create a standard command pool that is maintained by this device.
		}

		void Device::one_time_submit(QueueType type, const CommandBuffer& command_buffer)
		{
			if (command_buffer.is_inside_render_pass())
			{
				throw std::runtime_error("The command buffer passed to `one_time_submit()` is still inside a render pass: call `end_render_pass()`\n");
			}
			if (command_buffer.is_recording())
			{
				throw std::runtime_error("The command buffer passed to `one_time_submit()` is still in a recording state: call `end()`");
			}

			vk::CommandBuffer command_buffer_handle = command_buffer.get_handle();

			vk::SubmitInfo submit_info = {};
			submit_info.commandBufferCount = 1;
			submit_info.pCommandBuffers = &command_buffer_handle;

			get_queue_handle(type).submit(submit_info, {});
			get_queue_handle(type).waitIdle();
		}

		void Device::present(const Swapchain& swapchain, uint32_t image_index, const Semaphore& wait)
		{
			std::vector<vk::SwapchainKHR> swapchain_handles = { swapchain.get_handle() };

			vk::PresentInfoKHR present_info = {};
			present_info.waitSemaphoreCount = 1;
			present_info.pWaitSemaphores = &wait.get_handle();
			present_info.swapchainCount = static_cast<uint32_t>(swapchain_handles.size());
			present_info.pSwapchains = swapchain_handles.data();
			present_info.pImageIndices = &image_index;
			present_info.pResults = nullptr;

			get_queue_handle(QueueType::PRESENTATION).presentKHR(present_info);
		}

		std::ostream& operator<<(std::ostream& stream, const Device& device)
		{
			stream << "Device object: " << device.m_device_handle.get() << std::endl;
			stream << "Chosen physical device object: " << device.m_gpu_details.m_handle << std::endl;
			std::cout << "\tDevice ID: " << device.m_gpu_details.m_properties.deviceID << std::endl;
			std::cout << "\tDevice name: " << device.m_gpu_details.m_properties.deviceName << std::endl;
			std::cout << "\tVendor ID: " << device.m_gpu_details.m_properties.vendorID << std::endl;

			stream << "Queue family details:" << std::endl;
			stream << "\tQueue family - graphics index: " << device.m_queue_families_mapping.at(QueueType::GRAPHICS).index << std::endl;
			stream << "\tQueue family - compute index: " << device.m_queue_families_mapping.at(QueueType::COMPUTE).index << std::endl;
			stream << "\tQueue family - transfer index: " << device.m_queue_families_mapping.at(QueueType::TRANSFER).index << std::endl;
			stream << "\tQueue family - sparse binding index: " << device.m_queue_families_mapping.at(QueueType::SPARSE_BINDING).index << std::endl;
			stream << "\tQueue family - present index: " << device.m_queue_families_mapping.at(QueueType::PRESENTATION).index << std::endl;

			return stream;
		}

	} // namespace graphics

} // namespace plume