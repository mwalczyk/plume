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
		m_border_color = vk::BorderColor::eIntOpaqueBlack;
		m_mipmap_mode = vk::SamplerMipmapMode::eLinear;
		m_unnormalized_coordinates = VK_FALSE;
		m_compare_op_enable = VK_FALSE;
		m_compare_op = vk::CompareOp::eAlways;
	}

	Sampler::Sampler(DeviceWeakRef device, const Options& options) :
		m_device(device)
	{
		DeviceRef device_shared = m_device.lock();

		vk::SamplerCreateInfo sampler_create_info;
		sampler_create_info.addressModeU = options.m_address_mode_u;
		sampler_create_info.addressModeV = options.m_address_mode_v;
		sampler_create_info.addressModeW = options.m_address_mode_w;
		sampler_create_info.anisotropyEnable = options.m_anistropy_enabled;
		sampler_create_info.borderColor = options.m_border_color;
		sampler_create_info.compareEnable = options.m_compare_op_enable; 
		sampler_create_info.compareOp = options.m_compare_op;
		sampler_create_info.magFilter = options.m_mag_filter;
		sampler_create_info.maxAnisotropy = options.m_max_anistropy;
		sampler_create_info.maxLod = options.m_max_lod;
		sampler_create_info.minFilter = options.m_min_filter;
		sampler_create_info.minLod = options.m_min_lod;
		sampler_create_info.mipLodBias = options.m_mip_lod_bias;
		sampler_create_info.mipmapMode = options.m_mipmap_mode;
		sampler_create_info.unnormalizedCoordinates = options.m_unnormalized_coordinates;

		m_sampler_handle = device_shared->get_handle().createSampler(sampler_create_info);
	}

	Sampler::~Sampler()
	{
		DeviceRef device_shared = m_device.lock();

		device_shared->get_handle().destroySampler(m_sampler_handle);
	}

	static const vk::ImageUsageFlags IMAGE_USAGE_ALL = vk::ImageUsageFlagBits::eColorAttachment |
		vk::ImageUsageFlagBits::eDepthStencilAttachment |
		vk::ImageUsageFlagBits::eInputAttachment |
		vk::ImageUsageFlagBits::eSampled |
		vk::ImageUsageFlagBits::eStorage |
		vk::ImageUsageFlagBits::eTransferDst |
		vk::ImageUsageFlagBits::eTransferSrc |
		vk::ImageUsageFlagBits::eTransientAttachment;

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

		// TODO: images should be able to be shared across multiple queue families.
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

		// If the image was constructed with more than one array layer, set this flag to true
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
			// Note that it is not possible to have an array of 3D textures
			return vk::ImageViewType::e3D;

		// TODO: cubemaps and cubemap arrays
		}
	}

	ImageView::ImageView(DeviceWeakRef device, ImageWeakRef image, uint32_t base_array_layer, uint32_t layer_count, uint32_t base_mip_level, uint32_t level_count)
	{
		vk::ImageSubresourceRange subresource_range = {
			utils::format_to_aspect_mask(image.lock()->m_format),
			base_mip_level,
			level_count,
			base_array_layer,
			layer_count
		};

		ImageView(device, image, subresource_range);
	}

	ImageView::ImageView(DeviceWeakRef device, ImageWeakRef image, const vk::ImageSubresourceRange& subresource_range) :
		m_device(device),
		m_image(image),
		m_subresource_range(subresource_range)
	{
		DeviceRef device_shared = m_device.lock();

		if (image.expired())
		{
			throw std::runtime_error("Attempting to build an image view for an image that has already expired");
		}

		// In order to use the contents of the weak pointer, we must convert it to a shared pointer. At this 
		// point, we have already verified that the parent image has not yet expired, so this is a safe operation.
		auto shared_image_lock = image.lock();

		if (!shared_image_lock->m_is_array && m_subresource_range.layerCount > 1)
		{
			throw std::runtime_error("Attempting to build an image view that accesses multiple array layers \
				of the parent image, but the parent image is not an array");
		}

		vk::ImageViewCreateInfo image_view_create_info;
		image_view_create_info.format = shared_image_lock->m_format;
		image_view_create_info.image = shared_image_lock->m_image_handle;
		image_view_create_info.subresourceRange.aspectMask = m_subresource_range.aspectMask;
		image_view_create_info.subresourceRange.baseArrayLayer = m_subresource_range.baseArrayLayer;
		image_view_create_info.subresourceRange.baseMipLevel = m_subresource_range.baseMipLevel;
		image_view_create_info.subresourceRange.layerCount = m_subresource_range.layerCount;
		image_view_create_info.subresourceRange.levelCount = m_subresource_range.levelCount;
		image_view_create_info.viewType = shared_image_lock->image_view_type_from_parent();

		m_image_view_handle = device_shared->get_handle().createImageView(image_view_create_info);
	}

	ImageView::~ImageView()
	{
		DeviceRef device_shared = m_device.lock();

		device_shared->get_handle().destroyImageView(m_image_view_handle);
	}

} // namespace graphics