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

#include "DeviceMemory.h"
#include "ResourceManager.h"
#include "Sampler.h"
#include "Utils.h"

namespace graphics
{

	class Image
	{
	public:

		//! Factory method for returning a new ImageRef whose device local memory store will be uninitialized. Note that
		//! this image is not host accessible.
		Image(const Device& device,
			  vk::ImageType image_type, 
			  vk::ImageUsageFlags image_usage_flags,
			  vk::Format format, 
			  vk::Extent3D dimensions,
			  uint32_t array_layers = 1,
			  uint32_t mip_levels = 1, 
			  vk::ImageTiling image_tiling = vk::ImageTiling::eOptimal, 
			  uint32_t sample_count = 1);

		//! Factory method for returning a new ImageRef that will be pre-initialized with the user supplied data. The resulting 
		//! image will be 2D with depth, array layers, and mipmap levels equal to 1.
		template<typename T>
		Image(const Device& device, 
			  vk::ImageType image_type, 
			  vk::ImageUsageFlags image_usage_flags, 
			  vk::Format format, 
			  vk::Extent3D dimensions, 
			  const std::vector<T>& pixels) :

			m_device_ptr(&device),
			m_image_type(image_type),
			m_image_usage_flags(image_usage_flags),
			m_format(format),
			m_dimensions(dimensions),
			m_array_layers(1),
			m_mip_levels(1),
			m_image_tiling(vk::ImageTiling::eLinear),
			m_sample_count(vk::SampleCountFlagBits::e1),
			m_current_layout(vk::ImageLayout::ePreinitialized),
			m_is_host_accessible(true)
		{
			check_image_parameters();

			vk::ImageCreateInfo image_create_info;
			image_create_info.arrayLayers = m_array_layers;
			image_create_info.extent = m_dimensions;
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

			initialize_device_memory_with_flags(device, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
			
			// Fill the first array layer and mipmap level.
			vk::ImageSubresource image_subresource;
			image_subresource.aspectMask = utils::format_to_aspect_mask(m_format);
			image_subresource.arrayLayer = 0;
			image_subresource.mipLevel = 0;

			vk::SubresourceLayout subresource_layout = m_device_ptr->get_handle().getImageSubresourceLayout(m_image_handle.get(), image_subresource);

			// We assume that the image data has four channels (RGBA). We also need to account for the bit depth
			// of the image data: floats are 4 bits, for example.
			const size_t channels = 4;
			constexpr size_t bit_multipler = sizeof(T);

			// Fill the image with the provided data.
			void* mapped_ptr = m_device_memory->map(0, m_device_memory->get_allocation_size());		

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

		//! Factory method for returning a new ImageRef from an LDR image file. The resulting image will be 2D
		//! with depth, array layers, and mipmap levels equal to 1.
		Image(const Device& device, 
			  vk::ImageType image_type,
			  vk::ImageUsageFlags image_usage_flags, 
			  vk::Format format, 
			  const fsys::ImageResource& resource) :

			Image(device, image_type, image_usage_flags, format, { resource.width, resource.height, 1 }, resource.contents) {}

		//! Factory method for returning a new ImageRef from an HDR image file. The resulting image will be 2D
		//! with depth, array layers, and mipmap levels equal to 1.
		Image(const Device& device, 
			  vk::ImageType image_type, 
			  vk::ImageUsageFlags image_usage_flags, 
			  vk::Format format, 
			  const fsys::ImageResourceHDR& resource) :

			Image(device, image_type, image_usage_flags, format, { resource.width, resource.height, 1 }, resource.contents) {}

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

		vk::Image get_handle() const { return m_image_handle.get(); }

		//! Returns the type of this image (for example, vk::ImageType::e2D).
		vk::ImageType get_image_type() const { return m_image_type; }

		//! Returns the usage flag(s) that were used to create this image.
		vk::ImageUsageFlags get_image_usage_flags() const { return m_image_usage_flags; }

		//! Returns the internal format of this image (for example, vk::Format::eB8G8R8A8Unorm).
		vk::Format get_format() const { return m_format; }
		
		//! Returns a vk::Extent3D struct representing the width, height, and depth of the image.
		vk::Extent3D get_dimensions() const { return m_dimensions; }

		//! Returns `true` if this image has more than 1 array layer and `false` otherwise.
		bool is_array() const { return m_array_layers > 1; }

		//! Returns the number of array layers in this image.
		uint32_t get_array_layers() const { return m_array_layers; }

		//! Returns `true` if this image has more than 1 mipmap level and `false` otherwise.
		bool is_mipmapped() const { return m_mip_levels > 1; }

		//! Returns the number of mipmap levels in this image.
		uint32_t get_mip_levels() const { return m_mip_levels; }

		//! Returns the tiling mode of this image (for example, vk::ImageTiling::eLinear).
		vk::ImageTiling get_image_tiling() const { return m_image_tiling; }
		
		//! Returns `true` if the image has vk::SampleCountFlags greater than 1 and `false` otherwise.
		bool is_multisampled() const { return m_sample_count != vk::SampleCountFlagBits::e1; }

		//! Returns the sample count flags used to construct this image.
		vk::SampleCountFlags get_sample_count() const { return m_sample_count; }

		//! Returns the image create flags used to construct this image.
		vk::ImageCreateFlags get_image_create_flags() const { return m_image_create_flags; }

		//! Returns the current layout of the image.
		vk::ImageLayout get_current_layout() const { return m_current_layout; }

		//! Returns `true` if the vk::ImageViewType is compatible with the parent image type and `false` otherwise.
		bool is_image_view_type_compatible(vk::ImageViewType image_view_type) const;

		//! Returns `true` if the device memory backed by this image can be mapped by the application and `false` otherwise.
		bool is_host_accessible() const { return m_is_host_accessible; }

	private:

		//! Given the memory requirements of this image, allocate the appropriate type and size of device memory.
		void initialize_device_memory_with_flags(const Device& device, vk::MemoryPropertyFlags memory_property_flags);

		//! Prior to image creation, verify that the parameters passed to the constructor are valid.
		void check_image_parameters();

		//! Called by the CommandBuffer class during image transitions to update this image's current layout.
		void set_current_layout(vk::ImageLayout layout) { m_current_layout = layout; }

		const Device* m_device_ptr;
		vk::UniqueImage m_image_handle;
		std::unique_ptr<DeviceMemory> m_device_memory;

		vk::ImageType m_image_type;
		vk::ImageUsageFlags m_image_usage_flags;
		vk::Format m_format;
		vk::Extent3D m_dimensions;
		uint32_t m_array_layers;
		uint32_t m_mip_levels;
		vk::ImageTiling m_image_tiling;
		vk::SampleCountFlagBits m_sample_count;
		vk::ImageCreateFlags m_image_create_flags;
		mutable vk::ImageLayout m_current_layout;
		bool m_is_host_accessible;
		
		friend class ImageView;
		friend class CommandBuffer;
	};

	//! Note that in Vulkan an image view only remains valid for as long as the parent image is alive.
	//! Therefore, it makes sense to use a reference-counted pointer here.
	class ImageView 
	{
	public:

		//! An enum used by the `get_component_mapping_preset()` function to create a vk::ComponentMapping struct.
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

		ImageView(const Device& device, 
				  const Image& image, 
				  vk::ImageViewType image_view_type = vk::ImageViewType::e2D,
				  const vk::ImageSubresourceRange& subresource_range = Image::build_single_layer_subresource(),
				  const vk::ComponentMapping& component_mapping = get_component_mapping_preset());

		//! Builds a vk::DescriptorImageInfo struct corresponding to this image view and a corresponding sampler. The
		//! `image_layout` parameter specifies the layout that the parent image will be in when this descriptor is accessed.
		vk::DescriptorImageInfo build_descriptor_info(const Sampler& sampler, vk::ImageLayout image_layout = vk::ImageLayout::eShaderReadOnlyOptimal) const
		{
			return{ sampler.get_handle(), m_image_view_handle.get(), image_layout };
		}

		vk::ImageView get_handle() const { return m_image_view_handle.get(); }

		//! Returns the type of this image view (for example, vk::ImageViewType::e2D).
		vk::ImageViewType get_image_view_type() const { return m_image_view_type; }

		//! Returns the vk::ImageSubresourceRange that was used to construct this ImageView. This struct contains information
		//! about the layers and mipmap levels of the parent image that this ImageView accesses.
		const vk::ImageSubresourceRange& get_subresource_range() const { return m_subresource_range; }

	private:

		const Device* m_device_ptr;
		vk::UniqueImageView m_image_view_handle;

		vk::ImageViewType m_image_view_type;
		vk::ImageSubresourceRange m_subresource_range;
	};

} // namespace graphics
