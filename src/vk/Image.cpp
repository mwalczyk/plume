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

#include "Image.h"

namespace plume
{

	namespace graphics
	{

		void Image::initialize_device_memory_with_flags(const Device& device, vk::MemoryPropertyFlags memory_property_flags)
		{
			// Retrieve the memory requirements for this image.
			auto memory_requirements = m_device_ptr->get_handle().getImageMemoryRequirements(m_image_handle.get());
			auto required_memory_properties = memory_property_flags;

			// Allocate device memory.
			m_device_memory = std::make_unique<DeviceMemory>(device, memory_requirements, required_memory_properties);

			// Associate the device memory with this image.
			m_device_ptr->get_handle().bindImageMemory(m_image_handle.get(), m_device_memory->get_handle(), 0);
		}

		Image::Image(const Device& device,
			vk::ImageType image_type,
			vk::ImageUsageFlags image_usage_flags,
			vk::Format format,
			vk::Extent3D dimensions,
			uint32_t array_layers,
			uint32_t mip_levels,
			vk::ImageTiling image_tiling,
			uint32_t sample_count) :

			m_device_ptr(&device),
			m_image_type(image_type),
			m_image_usage_flags(image_usage_flags),
			m_format(format),
			m_dimensions(dimensions),
			m_array_layers(array_layers),
			m_mip_levels(mip_levels),
			m_image_tiling(image_tiling),
			m_sample_count(utils::sample_count_to_flags(sample_count)),
			m_current_layout(vk::ImageLayout::eUndefined),
			m_is_host_accessible(false)
		{
			check_image_parameters();

			// TODO: images should be able to be shared across multiple queue families. This should also be verified
			// in the CommandBuffer `transition_image_layout()` function.

			vk::ImageCreateInfo image_create_info;
			image_create_info.arrayLayers = m_array_layers;
			image_create_info.extent = m_dimensions;
			image_create_info.flags = m_image_create_flags;
			image_create_info.format = m_format;
			image_create_info.initialLayout = m_current_layout;
			image_create_info.imageType = m_image_type;
			image_create_info.mipLevels = m_mip_levels;
			image_create_info.pQueueFamilyIndices = nullptr;
			image_create_info.queueFamilyIndexCount = 0;
			image_create_info.samples = m_sample_count;
			image_create_info.sharingMode = (image_create_info.pQueueFamilyIndices) ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
			image_create_info.tiling = m_image_tiling;
			image_create_info.usage = m_image_usage_flags;

			m_image_handle = m_device_ptr->get_handle().createImageUnique(image_create_info);

			initialize_device_memory_with_flags(device, vk::MemoryPropertyFlagBits::eDeviceLocal);
		}

		bool Image::is_image_view_type_compatible(vk::ImageViewType image_view_type) const
		{
			// See the spec: https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#resources-image-views-compatibility

			switch (image_view_type)
			{
			case vk::ImageViewType::e1D:
				if (m_image_type != vk::ImageType::e1D) return false;
				break;
			case vk::ImageViewType::e1DArray:
				if (m_image_type != vk::ImageType::e1D || !(m_array_layers > 1)) return false;
				break;
			case vk::ImageViewType::e2D:
				if (m_image_type != vk::ImageType::e2D) return false;
				break;
			case vk::ImageViewType::e2DArray:
				if (m_image_type != vk::ImageType::e2D || !(m_array_layers > 1)) return false;
				break;
			case vk::ImageViewType::e3D:
				if (m_image_type != vk::ImageType::e3D) return false;
				break;
			case vk::ImageViewType::eCube:
			case vk::ImageViewType::eCubeArray:
			default:
				if (m_image_type != vk::ImageType::e2D || m_array_layers < 6 || m_dimensions.width != m_dimensions.height || !(m_image_create_flags & vk::ImageCreateFlagBits::eCubeCompatible)) return false;
				break;
			}

			return true;
		}

		void Image::check_image_parameters()
		{
			// See: https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#VkImageCreateInfo

			// General dimension checks.
			if (m_dimensions.width < 1 ||
				m_dimensions.height < 1 ||
				m_dimensions.depth < 1 ||
				m_array_layers < 1 ||
				m_mip_levels < 1)
			{
				throw std::runtime_error("Image created with width, height, depth, array layers, or mipmap levels less than 1");
			}

			// 1D image checks.
			if (m_image_type == vk::ImageType::e1D)
			{
				if (m_dimensions.height > 1 || m_dimensions.depth > 1)
				{
					throw std::runtime_error("Cannot create a 1D image with height or depth greater than 1");
				}
			}

			// 2D image checks.
			if (m_image_type == vk::ImageType::e2D)
			{
				if (m_dimensions.depth > 1)
				{
					throw std::runtime_error("Cannot create a 2D image with depth greater than 1");
				}
			}

			// 3D image checks.
			if (m_image_type == vk::ImageType::e3D)
			{
				if (m_array_layers > 1)
				{
					throw std::runtime_error("Cannot create a 3D image with array layers greater than 1");
				}
			}

			// Cube map checks.
			if (m_image_create_flags & vk::ImageCreateFlagBits::eCubeCompatible &&
				m_image_type == vk::ImageType::e2D)
			{
				if (m_dimensions.width != m_dimensions.height || m_array_layers < 6)
				{
					throw std::runtime_error("Image created with the vk::ImageCreateFlagBits::eCubeCompatible bit set, but the provided dimensions are not\
									      valid (width and height must be equal and array layers must be greater than or equal to 6");
				}
			}

			// MS checks.
			if (m_sample_count != vk::SampleCountFlagBits::e1)
			{
				// MS images must be 2D, cannot be cube compatible, must be optimal tiling, and cannot have multiple
				// mipmap levels.
				if (m_image_type != vk::ImageType::e2D ||
					m_image_create_flags & vk::ImageCreateFlagBits::eCubeCompatible ||
					m_image_tiling != vk::ImageTiling::eOptimal ||
					m_mip_levels != 1)
				{
					throw std::runtime_error("Image created with multiple samples, but one or more of the provided arguments are invalid");
				}
			}
		}

		ImageView::ImageView(const Device& device,
			const Image& image,
			vk::ImageViewType image_view_type,
			const vk::ImageSubresourceRange& subresource_range,
			const vk::ComponentMapping& component_mapping) :

			m_device_ptr(&device),
			m_subresource_range(subresource_range),
			m_image_view_type(image_view_type)
		{
			if (!image.is_image_view_type_compatible(m_image_view_type))
			{
				throw std::runtime_error("The requested image view type is not compatible with the parent image type");
			}

			vk::ImageViewCreateInfo image_view_create_info;
			image_view_create_info.format = image.m_format;
			image_view_create_info.image = image.get_handle();
			image_view_create_info.subresourceRange = m_subresource_range;
			image_view_create_info.components = component_mapping;
			image_view_create_info.viewType = m_image_view_type;

			// TODO: during the image view creation process, we make several assumptions. In particular,
			// we assume that the image view will have the same format as the parent image.
			if (image.m_format != image_view_create_info.format &&
				!(image.m_image_create_flags & vk::ImageCreateFlagBits::eMutableFormat))
			{
				throw std::runtime_error("Attempting to create an image view with a different format from the parent image, but the\
									  parent image was not created with the vk::ImageCreateFlagBits::eMutableFormat bit set");
			}

			m_image_view_handle = m_device_ptr->get_handle().createImageViewUnique(image_view_create_info);
		}

	} // namespace graphics

} // namespace plume