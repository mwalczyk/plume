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
#include <string>

#include "Platform.h"
#include "Noncopyable.h"
#include "Device.h"
#include "DeviceMemory.h"
#include "ResourceManager.h"
#include "Sampler.h"
#include "Utils.h"

namespace graphics
{

	class Image;
	using ImageRef = std::shared_ptr<Image>;
	using ImageWeakRef = std::weak_ptr<Image>;

	class Image : public Noncopyable
	{
	public:

		//! Factory method for returning a new ImageRef whose device local memory store will be empty.
		static ImageRef create(DeviceWeakRef device, 
							   vk::ImageType image_type, 
							   vk::ImageUsageFlags image_usage_flags, 
							   vk::Format format, 
							   vk::Extent3D dimensions, 
							   uint32_t mip_levels = 1,
							   vk::ImageTiling image_tiling = vk::ImageTiling::eLinear,
							   uint32_t sample_count = 1)
		{
			return std::make_shared<Image>(device, image_type, image_usage_flags, format, dimensions, mip_levels, image_tiling, sample_count);
		}

		//! Factory method for returning a new ImageRef that will be pre-initialized with the user supplied data.
		template<class T>
		static ImageRef create(DeviceWeakRef device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, const std::vector<T>& data)
		{
			return std::make_shared<Image>(device, image_type, image_usage_flags, format, width, height, depth, mip_levels, data);
		}

		//! Factory method for returning a new ImageRef from an LDR image file.
		static ImageRef create(DeviceWeakRef device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, const ImageResource& resource)
		{
			return std::make_shared<Image>(device, image_type, image_usage_flags, format, resource);
		}

		//! Factory method for returning a new ImageRef from an HDR image file.
		static ImageRef create(DeviceWeakRef device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, const ImageResourceHDR& resource)
		{
			return std::make_shared<Image>(device, image_type, image_usage_flags, format, resource);
		}

		Image(DeviceWeakRef device,
			  vk::ImageType image_type, 
			  vk::ImageUsageFlags image_usage_flags,
			  vk::Format format, 
			  vk::Extent3D dimensions,
			  uint32_t mip_levels = 1, 
			  vk::ImageTiling image_tiling = vk::ImageTiling::eLinear, 
			  uint32_t sample_count = 1);

		template<typename T>
		Image(DeviceWeakRef device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, vk::Extent3D dimensions, uint32_t mip_levels, const std::vector<T>& pixels) :
			m_device(device),
			m_image_type(image_type),
			m_image_usage_flags(image_usage_flags),
			m_format(format),
			m_dimensions(dimensions),
			m_mip_levels(mip_levels),
			m_is_array(false)
		{
			DeviceRef device_shared = m_device.lock();

			m_current_layout = vk::ImageLayout::ePreinitialized;

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
			image_create_info.samples = vk::SampleCountFlagBits::e1;
			image_create_info.sharingMode = (image_create_info.pQueueFamilyIndices) ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
			image_create_info.tiling = vk::ImageTiling::eLinear;
			image_create_info.usage = m_image_usage_flags;

			m_image_handle = device_shared->get_handle().createImage(image_create_info);

			initialize_device_memory_with_flags(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

			// Fill the image with the provided data.
			void* mapped_ptr = m_device_memory->map(0, m_device_memory->get_allocation_size());		// Map

			vk::ImageSubresource image_subresource;
			image_subresource.aspectMask = utils::format_to_aspect_mask(m_format);
			image_subresource.arrayLayer = 0;
			image_subresource.mipLevel = 0;

			vk::SubresourceLayout subresource_layout = device_shared->get_handle().getImageSubresourceLayout(m_image_handle, image_subresource);

			// We assume that the image data has four channels (RGBA). We also need to account for the bit depth
			// of the image data: floats are 4 bits, for example.
			const size_t channels = 4;
			constexpr size_t bit_multipler = sizeof(T);

			// The subresource has no additional padding, so we can directly copy the pixel data into the image.
			// This usually happens when the requested image is a power-of-two texture.
			VkDeviceSize image_size = m_dimensions.width * m_dimensions.height * channels;
			if (subresource_layout.rowPitch == m_dimensions.width * channels)
			{
				memcpy(mapped_ptr, pixels.data(), static_cast<size_t>(image_size));
			}
			else
			{
				uint8_t* data_as_bytes = reinterpret_cast<uint8_t*>(mapped_ptr);
				for (size_t i = 0; i < m_dimensions.height; ++i)
				{
					memcpy(&data_as_bytes[i * subresource_layout.rowPitch], &pixels[i * m_dimensions.width * channels], m_dimensions.width * channels * bit_multipler);
				}
			}

			m_device_memory->unmap();
		}

		Image(DeviceWeakRef device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, const ImageResource& resource) :
			Image(device, image_type, image_usage_flags, format, { resource.width, resource.height, 1 }, 1, resource.contents) {}

		Image(DeviceWeakRef device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, const ImageResourceHDR& resource) :
			Image(device, image_type, image_usage_flags, format, { resource.width, resource.height, 1 }, 1, resource.contents) {}

		~Image();

		//! Helper function for creating an image subresource range that corresponds to the first layer 
		//! and mipmap level of an arbitrary image.
		static vk::ImageSubresourceRange build_single_layer_subresource(vk::ImageAspectFlags image_aspect_flags = vk::ImageAspectFlagBits::eColor)
		{
			vk::ImageSubresourceRange image_subresource = {};
			image_subresource.aspectMask = image_aspect_flags;
			image_subresource.baseArrayLayer = 0;
			image_subresource.layerCount = 1;
			image_subresource.baseMipLevel = 0;
			image_subresource.levelCount = 1;

			return image_subresource;
		}

		//! Helper function for creating an image subresource range that involves multiple layers and/or 
		//! mipmap levels of an arbitrary image.
		static vk::ImageSubresourceRange build_multiple_layer_subresource(uint32_t base_layer, 
																	      uint32_t layer_count, 
																		  uint32_t base_level = 0,
																		  uint32_t level_count = 1,
																		  vk::ImageAspectFlags image_aspect_flags = vk::ImageAspectFlagBits::eColor)
		{
			vk::ImageSubresourceRange image_subresource = {};
			image_subresource.aspectMask = image_aspect_flags;
			image_subresource.baseArrayLayer = base_layer;
			image_subresource.layerCount = layer_count;
			image_subresource.baseMipLevel = base_level;
			image_subresource.levelCount = level_count;

			return image_subresource;
		}

		vk::Image get_handle() const { return m_image_handle; }

		vk::Format get_format() const { return m_format; }

		vk::ImageLayout get_current_layout() const { return m_current_layout; }

	private:

		//! Given the memory requirements of this image, allocate the appropriate type and size of device memory.
		void initialize_device_memory_with_flags(vk::MemoryPropertyFlags memory_property_flags);

		vk::ImageViewType image_view_type_from_parent() const;

		DeviceWeakRef m_device;
		DeviceMemoryRef m_device_memory;
		vk::Image m_image_handle;
		vk::ImageType m_image_type;
		vk::ImageUsageFlags m_image_usage_flags;
		vk::Format m_format;
		vk::ImageLayout m_current_layout;
		vk::Extent3D m_dimensions;
		uint32_t m_channels;
		uint32_t m_mip_levels;
		vk::ImageTiling m_image_tiling;
		vk::SampleCountFlagBits m_sample_count;
		bool m_is_array;

		friend class ImageView;
		friend class CommandBuffer;
	};

	class ImageView;
	using ImageViewRef = std::shared_ptr<ImageView>;

	//! Note that in Vulkan an image view only remains valid for as long as the parent image is alive.
	//! Therefore, it makes sense to use a reference-counted pointer here.
	class ImageView : public Noncopyable
	{
	public:

		enum class ComponentMappingPreset
		{
			MAPPING_IDENTITY,
			MAPPING_RGB,
			MAPPING_RBG,
			MAPPING_BRG,
			MAPPING_BGR,
			MAPPING_GRB,
			MAPPING_GBR
		};

		//! A helper function for constructing a vk::ComponentMapping, which is supplied
		//! to the ImageView constructor.
		static vk::ComponentMapping get_component_mapping_preset(ComponentMappingPreset preset = ComponentMappingPreset::MAPPING_IDENTITY)
		{
			switch (preset)
			{
			case ComponentMappingPreset::MAPPING_IDENTITY:
			case ComponentMappingPreset::MAPPING_RGB:
				return{ vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity };
			case ComponentMappingPreset::MAPPING_RBG:
				return{ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eIdentity };
			case ComponentMappingPreset::MAPPING_BRG:
				return{ vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eIdentity };
			case ComponentMappingPreset::MAPPING_BGR:
				return{ vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eIdentity };
			case ComponentMappingPreset::MAPPING_GRB:
				return{ vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eIdentity };
			case ComponentMappingPreset::MAPPING_GBR:
			default:
				return{ vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eIdentity };
			}
		}

		//! Factory method for returning a new ImageViewRef from a parent ImageRef and a
		//! number of details related to a particular subresource range of the parent image.
		static ImageViewRef create(DeviceWeakRef device, 
			ImageRef image, 
			uint32_t base_array_layer = 0, 
			uint32_t layer_count = 1, 
			uint32_t base_mip_level = 0,
			uint32_t level_count = 1,
			const vk::ComponentMapping& component_mapping = get_component_mapping_preset())
		{
			vk::ImageSubresourceRange subresource_range = 
			{
				utils::format_to_aspect_mask(image->m_format),
				base_mip_level, 
				level_count,
				base_array_layer, 
				layer_count
			};

			return std::make_shared<ImageView>(device, image, subresource_range, component_mapping);
		}

		//! Factory method for returning a new ImageViewRef from a parent ImageRef and a 
		//! vk::ImageSubresourceRange.
		static ImageViewRef create(DeviceWeakRef device, 
			ImageRef image, 
			const vk::ImageSubresourceRange& subresource_range,
			const vk::ComponentMapping& component_mapping = get_component_mapping_preset())
		{
			return std::make_shared<ImageView>(device, image, subresource_range, component_mapping);
		}

		ImageView(DeviceWeakRef device, 
			ImageRef image, 
			uint32_t base_array_layer = 0, 
			uint32_t layer_count = 1, 
			uint32_t base_mip_level = 0, 
			uint32_t level_count = 1,
			const vk::ComponentMapping& component_mapping = get_component_mapping_preset());
		
		ImageView(DeviceWeakRef device, 
			ImageRef image, 
			const vk::ImageSubresourceRange& subresource_range,
			const vk::ComponentMapping& component_mapping = get_component_mapping_preset());
		
		~ImageView();

		vk::DescriptorImageInfo build_descriptor_info(const SamplerRef& sampler, vk::ImageLayout image_layout = vk::ImageLayout::eShaderReadOnlyOptimal) const
		{
			return{ sampler->get_handle(), m_image_view_handle, image_layout };
		}

		vk::ImageView get_handle() const { return m_image_view_handle; }

	private:

		DeviceWeakRef m_device;
		ImageRef m_image;
		vk::ImageSubresourceRange m_subresource_range;
		vk::ImageView m_image_view_handle;
	};

} // namespace graphics
