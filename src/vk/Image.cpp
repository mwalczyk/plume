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

	Sampler::Options::Options()
	{
		m_address_mode_u = m_address_mode_v = m_address_mode_w = vk::SamplerAddressMode::eRepeat;
		m_min_filter = m_mag_filter = vk::Filter::eLinear;
		m_min_lod = m_max_lod = m_mip_lod_bias = 0.0f;
		m_anistropy_enabled = VK_TRUE;
		m_max_anistropy = 16.0f;
	}

	Sampler::Sampler(const DeviceRef& device, const Options& options) :
		m_device(device)
	{
		vk::SamplerCreateInfo sampler_create_info;
		sampler_create_info.addressModeU = options.m_address_mode_u;
		sampler_create_info.addressModeV = options.m_address_mode_v;
		sampler_create_info.addressModeW = options.m_address_mode_w;
		sampler_create_info.anisotropyEnable = options.m_anistropy_enabled;
		sampler_create_info.borderColor = vk::BorderColor::eIntOpaqueBlack;
		sampler_create_info.compareEnable = VK_FALSE;
		sampler_create_info.compareOp = vk::CompareOp::eAlways;
		sampler_create_info.magFilter = options.m_mag_filter;
		sampler_create_info.maxAnisotropy = options.m_max_anistropy;
		sampler_create_info.maxLod = options.m_max_lod;
		sampler_create_info.minFilter = options.m_min_filter;
		sampler_create_info.minLod = options.m_min_lod;
		sampler_create_info.mipLodBias = options.m_mip_lod_bias;
		sampler_create_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
		sampler_create_info.unnormalizedCoordinates = VK_FALSE;

		m_sampler_handle = m_device->get_handle().createSampler(sampler_create_info);
	}

	Sampler::~Sampler()
	{
		m_device->get_handle().destroySampler(m_sampler_handle);
	}

	Image::~Image()
	{
		m_device->get_handle().destroyImage(m_image_handle);
	}

	void Image::initialize_device_memory_with_flags(vk::MemoryPropertyFlags memory_property_flags)
	{
		// Retrieve the memory requirements for this image.
		auto memory_requirements = m_device->get_handle().getImageMemoryRequirements(m_image_handle);
		auto required_memory_properties = memory_property_flags;

		// Allocate device memory.
		m_device_memory = DeviceMemory::create(m_device, memory_requirements, required_memory_properties);

		// Associate the device memory with this image.
		m_device->get_handle().bindImageMemory(m_image_handle, m_device_memory->get_handle(), 0);
	}

	Image::Options::Options()
	{
		m_image_tiling = vk::ImageTiling::eLinear;
		m_sample_count_flag_bits = vk::SampleCountFlagBits::e1;
		m_mip_levels = 1;
	}

	Image::Image(const DeviceRef& device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, uint32_t width, uint32_t height, uint32_t depth, const Options& options) :
		m_device(device),
		m_image_type(image_type),
		m_image_usage_flags(image_usage_flags),
		m_format(format),
		m_width(width),
		m_height(height),
		m_depth(depth),
		m_mip_levels(options.m_mip_levels)
	{
		m_current_layout = vk::ImageLayout::eUndefined;

		vk::ImageCreateInfo image_create_info;
		image_create_info.arrayLayers = 1;
		image_create_info.extent.width = m_width;
		image_create_info.extent.height = m_height;
		image_create_info.extent.depth = m_depth;
		image_create_info.format = m_format;
		image_create_info.initialLayout = m_current_layout;
		image_create_info.imageType = m_image_type;
		image_create_info.mipLevels = m_mip_levels;
		image_create_info.pQueueFamilyIndices = options.m_queue_family_indices.data();
		image_create_info.queueFamilyIndexCount = static_cast<uint32_t>(options.m_queue_family_indices.size());
		image_create_info.samples = options.m_sample_count_flag_bits;
		image_create_info.sharingMode = (image_create_info.pQueueFamilyIndices) ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
		image_create_info.tiling = options.m_image_tiling;
		image_create_info.usage = m_image_usage_flags;

		m_image_handle = m_device->get_handle().createImage(image_create_info);

		initialize_device_memory_with_flags(vk::MemoryPropertyFlagBits::eDeviceLocal);
	}

	Image::Image(const DeviceRef& device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, const ImageResource& resource, const Options& options) :
		m_device(device),
		m_image_type(image_type),
		m_image_usage_flags(image_usage_flags),
		m_format(format),
		m_width(resource.width),
		m_height(resource.height),
		m_depth(1),
		m_mip_levels(options.m_mip_levels)
	{	
		m_current_layout = vk::ImageLayout::ePreinitialized;

		vk::ImageCreateInfo image_create_info;
		image_create_info.arrayLayers = 1;
		image_create_info.extent.width = m_width;
		image_create_info.extent.height = m_height;
		image_create_info.extent.depth = m_depth;
		image_create_info.format = m_format;
		image_create_info.initialLayout = m_current_layout;
		image_create_info.imageType = m_image_type;
		image_create_info.mipLevels = m_mip_levels;
		image_create_info.pQueueFamilyIndices = options.m_queue_family_indices.data();
		image_create_info.queueFamilyIndexCount = static_cast<uint32_t>(options.m_queue_family_indices.size());
		image_create_info.samples = options.m_sample_count_flag_bits;
		image_create_info.sharingMode = (image_create_info.pQueueFamilyIndices) ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
		image_create_info.tiling = options.m_image_tiling;
		image_create_info.usage = m_image_usage_flags; 

		m_image_handle = m_device->get_handle().createImage(image_create_info);
		
		initialize_device_memory_with_flags(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	
		// Fill the image with the provided data.
		void* mapped_ptr = m_device_memory->map(0, m_device_memory->get_allocation_size());		// Map

		vk::ImageSubresource image_subresource;
		image_subresource.aspectMask = utils::format_to_aspect_mask(m_format);
		image_subresource.arrayLayer = 0;
		image_subresource.mipLevel = 0;

		vk::SubresourceLayout subresource_layout = m_device->get_handle().getImageSubresourceLayout(m_image_handle, image_subresource);
			
		// The subresource has no additional padding, so we can directly copy the pixel data into the image.
		// This usually happens when the requested image is a power-of-two texture.
		VkDeviceSize image_size = m_width * m_height * 4;
		if (subresource_layout.rowPitch == m_width * 4)
		{
			memcpy(mapped_ptr, resource.contents.data(), static_cast<size_t>(image_size));
		}
		else 
		{
			uint8_t* data_as_bytes = reinterpret_cast<uint8_t*>(mapped_ptr);
			for (size_t i = 0; i < m_height; ++i)
			{
				memcpy(&data_as_bytes[i * subresource_layout.rowPitch], &resource.contents[i * m_width * 4], m_width * 4);
			}
		}
			
		m_device_memory->unmap();																// Unmap
	}

	vk::ImageView Image::build_image_view() const
	{
		vk::ImageViewCreateInfo image_view_create_info;
		image_view_create_info.format = m_format;
		image_view_create_info.image = m_image_handle;
		image_view_create_info.subresourceRange.aspectMask = utils::format_to_aspect_mask(m_format);
		image_view_create_info.subresourceRange.baseArrayLayer = 0;
		image_view_create_info.subresourceRange.baseMipLevel = 0;
		image_view_create_info.subresourceRange.layerCount = 1;
		image_view_create_info.subresourceRange.levelCount = 1;
		image_view_create_info.viewType = vk::ImageViewType::e2D;
		
		return m_device->get_handle().createImageView(image_view_create_info);
	}

} // namespace graphics