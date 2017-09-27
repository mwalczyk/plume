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

#include "Instance.h"

namespace plume
{

	namespace graphics
	{

		enum class QueueType
		{
			GRAPHICS,
			COMPUTE,
			TRANSFER,
			SPARSE_BINDING,
			PRESENTATION
		};

		class CommandBuffer;
		class Semaphore;
		class Swapchain;

		class Device
		{
		public:

			struct SwapchainSupportDetails
			{
				vk::SurfaceCapabilitiesKHR m_capabilities;
				std::vector<vk::SurfaceFormatKHR> m_formats;
				std::vector<vk::PresentModeKHR> m_present_modes;
			};

			//! Construct a logical device around a physical device (GPU).
			Device(vk::PhysicalDevice physical_device,
				const vk::UniqueSurfaceKHR& surface,
				vk::QueueFlags required_queue_flags = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer,
				bool use_swapchain = true,
				const std::vector<const char*>& required_device_extensions = {});

			~Device();

			vk::Device get_handle() const { return m_device_handle.get(); };

			vk::PhysicalDevice get_physical_device_handle() const { return m_gpu_details.m_handle; }

			vk::PhysicalDeviceProperties get_physical_device_properties() const { return m_gpu_details.m_properties; }

			vk::PhysicalDeviceLimits get_physical_device_limits() const { return m_gpu_details.m_properties.limits; }

			vk::PhysicalDeviceFeatures get_physical_device_features() const { return m_gpu_details.m_features; }

			vk::PhysicalDeviceMemoryProperties get_physical_device_memory_properties() const { return m_gpu_details.m_memory_properties; }

			const std::vector<vk::QueueFamilyProperties>& get_physical_device_queue_family_properties() const { return m_gpu_details.m_queue_family_properties; }

			const std::vector<vk::ExtensionProperties>& get_physical_device_extension_properties() const { return m_gpu_details.m_extension_properties; }

			//! Format features are properties of the physical device.
			vk::FormatProperties get_physical_device_format_properties(vk::Format format) const
			{
				return m_gpu_details.m_handle.getFormatProperties(format);
			}

			//! Depth formats are not necessarily supported by the system. Retrieve the highest precision format available.
			vk::Format get_supported_depth_format() const
			{
				return m_gpu_details.get_supported_depth_format();
			}

			uint32_t get_queue_family_index(QueueType type) const { return m_queue_families_mapping.at(type).index; }

			vk::Queue get_queue_handle(QueueType type) { return m_queue_families_mapping[type].handle; }

			uint32_t acquire_next_swapchain_image(const Swapchain& swapchain, const Semaphore& semaphore, uint32_t timeout = std::numeric_limits<uint64_t>::max());

			void one_time_submit(QueueType type, std::function<void(const CommandBuffer&)> func);

			void one_time_submit(QueueType type, const CommandBuffer& command_buffer);

			void submit_with_semaphores(QueueType type,
				const CommandBuffer& command_buffer,
				const Semaphore& wait,
				const Semaphore& signal,
				vk::PipelineStageFlags pipeline_stage_flags = vk::PipelineStageFlagBits::eColorAttachmentOutput);

			void present(const Swapchain& swapchain, uint32_t image_index, const Semaphore& wait);

			//! Wait for all commands submitted on a particular queue to finish.
			void wait_idle_queue(QueueType type) { get_queue_handle(type).waitIdle(); }

			//! Wait for all commands submitted to all queues to finish.
			void wait_idle() { m_device_handle.get().waitIdle(); }

			//! Returns a structure that contains information related to the chosen physical device's swapchain support.
			SwapchainSupportDetails get_swapchain_support_details(const vk::UniqueSurfaceKHR& surface) const;

			friend std::ostream& operator<<(std::ostream& stream, const Device& device);

		private:

			//! A struct for aggregating all of the information about a particular physical device that is
			//! associated with this logical device.
			struct GPUDetails
			{
				vk::PhysicalDevice m_handle;

				vk::PhysicalDeviceProperties m_properties;

				vk::PhysicalDeviceFeatures m_features;

				vk::PhysicalDeviceMemoryProperties m_memory_properties;

				std::vector<vk::QueueFamilyProperties> m_queue_family_properties;

				std::vector<vk::ExtensionProperties> m_extension_properties;

				vk::FormatProperties get_physical_device_format_properties(vk::Format format) const
				{
					return m_handle.getFormatProperties(format);
				}

				vk::Format get_supported_depth_format() const
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
			};

			//! A struct for aggregating all of the information about a particular queue family.
			struct QueueInternals
			{
				uint32_t index = 0;
				vk::Queue handle = {};
			};

			std::map<QueueType, QueueInternals> m_queue_families_mapping =
			{
				{ QueueType::GRAPHICS, {} },
				{ QueueType::COMPUTE, {} },
				{ QueueType::TRANSFER, {} },
				{ QueueType::SPARSE_BINDING, {} },
				{ QueueType::PRESENTATION, {} }
			};

			uint32_t find_queue_family_index(vk::QueueFlagBits queue_flag_bits) const;

			vk::UniqueDevice m_device_handle;

			GPUDetails m_gpu_details;
			std::vector<const char*> m_required_device_extensions;
		};

	} // namespace graphics

} // namespace plume