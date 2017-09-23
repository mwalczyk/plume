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

namespace graphics
{

	Image::~Image()
	{
		DeviceRef device_shared = m_device.lock();

		device_shared->get_handle().destroyImage(m_image_handle);
	}

	void Image::initialize_device_memory_with_flags(vk::MemoryPropertyFlags memory_property_flags)
	{
		DeviceRef device_shared = m_device.lock();

		// Retrieve the memory requirements for this image.
		auto memory_requirements = device_shared->get_handle().getImageMemoryRequirements(m_image_handle);
		auto required_memory_properties = memory_property_flags;

		// Allocate device memory.
		m_device_memory = DeviceMemory::create(device_shared, memory_requirements, required_memory_properties);

		// Associate the device memory with this image.
		device_shared->get_handle().bindImageMemory(m_image_handle, m_device_memory->get_handle(), 0);
	}

	Image::Image(DeviceWeakRef device,
				 vk::ImageType image_type,
				 vk::ImageUsageFlags image_usage_flags,
				 vk::Format format,
				 vk::Extent3D dimensions,
				 uint32_t mip_levels,
				 vk::ImageTiling image_tiling,
				 uint32_t sample_count) :

		m_device(device),
		m_image_type(image_type),
		m_image_usage_flags(image_usage_flags),
		m_format(format),
		m_dimensions(dimensions),
		m_mip_levels(mip_levels),
		m_image_tiling(image_tiling),
		m_sample_count(utils::sample_count_to_flags(sample_count))
	{
		DeviceRef device_shared = m_device.lock();

		m_current_layout = vk::ImageLayout::eUndefined;

		// TODO: images should be able to be shared across multiple queue families. This should also be verified
		// in the CommandBuffer `transition_image_layout()` function.

		vk::ImageCreateInfo image_create_info;
		image_create_info.arrayLayers = 1;
		image_create_info.extent.width = m_dimensions.width;
		image_create_info.extent.height = m_dimensions.height;
		image_create_info.extent.depth = m_dimensions.depth;
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

		m_image_handle = device_shared->get_handle().createImage(image_create_info);

		initialize_device_memory_with_flags(vk::MemoryPropertyFlagBits::eDeviceLocal);

		// If the image was constructed with more than one array layer, set this flag to `true`.
		m_is_array = image_create_info.arrayLayers > 1;
	}

	vk::ImageViewType Image::image_view_type_from_parent() const
	{
		switch (m_image_type)
		{
		case vk::ImageType::e1D:
			return m_is_array ? vk::ImageViewType::e1DArray : vk::ImageViewType::e1D;
		case vk::ImageType::e2D:
			return m_is_array ? vk::ImageViewType::e2DArray : vk::ImageViewType::e2D;
		case vk::ImageType::e3D:
		default:
			// Note that it is not possible to have an array of 3D textures.
			return vk::ImageViewType::e3D;

		// TODO: cubemaps and cubemap arrays - see the section of the spec dedicated
	    // to this: https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#resources-image-views-compatibility
		}
	}

	ImageView::ImageView(DeviceWeakRef device, 
						 ImageRef image, 
						 vk::ImageViewType image_view_type, 
						 uint32_t base_array_layer, 
						 uint32_t layer_count, 
						 uint32_t base_mip_level, 
						 uint32_t level_count,
						 const vk::ComponentMapping& component_mapping)
	{
		vk::ImageSubresourceRange subresource_range = 
		{
			utils::format_to_aspect_mask(image->m_format),
			base_mip_level,
			level_count,
			base_array_layer,
			layer_count
		};

		ImageView(device, image, image_view_type, subresource_range, component_mapping);
	}

	ImageView::ImageView(DeviceWeakRef device, 
						 ImageRef image, 
						 vk::ImageViewType image_view_type,
						 const vk::ImageSubresourceRange& subresource_range, 
						 const vk::ComponentMapping& component_mapping) :
		m_device(device),
		m_image(image),
		m_subresource_range(subresource_range),
		m_image_view_type(m_image_view_type)
	{
		DeviceRef device_shared = m_device.lock();

		if (!image->m_is_array && m_subresource_range.layerCount > 1)
		{
			throw std::runtime_error("Attempting to build an image view that accesses multiple array layers\
				of the parent image, but the parent image is not an array");
		}

		// TODO: during the image view creation process, we make several assumptions. In particular,
		// we assume that the image view will have the same format as the parent image.

		vk::ImageViewCreateInfo image_view_create_info;
		image_view_create_info.format = image->m_format;
		image_view_create_info.image = image->m_image_handle;
		image_view_create_info.subresourceRange = m_subresource_range;
		image_view_create_info.components = component_mapping;
		image_view_create_info.viewType = m_image_view_type;	

		m_image_view_handle = device_shared->get_handle().createImageView(image_view_create_info);
	}

	ImageView::~ImageView()
	{
		DeviceRef device_shared = m_device.lock();

		device_shared->get_handle().destroyImageView(m_image_view_handle);
	}

} // namespace graphics