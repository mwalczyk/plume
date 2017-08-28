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
#include "Utils.h"

namespace graphics
{

	class Sampler;
	using SamplerRef = std::shared_ptr<Sampler>;

	//! Image samplers are used by the implementation to read image data and apply filtering and other transformations
	//! inside of a shader. Note that a single sampler can be used with multiple attachments when constructing 
	//! descriptor sets. 
	class Sampler : public Noncopyable
	{
	public: 

		class Options
		{
		public:
			Options();

			//! Sets all three sampler address modes (u, v, w) simultaneously.
			Options& address_modes(vk::SamplerAddressMode mode) { m_address_mode_u = m_address_mode_v = m_address_mode_w = mode; return *this; }

			//! Sets each sampler address mode (u, v, w) independently.
			Options& address_modes(vk::SamplerAddressMode u, vk::SamplerAddressMode v, vk::SamplerAddressMode w) 
			{ 
				m_address_mode_u = u; 
				m_address_mode_v = v;
				m_address_mode_w = w;
				return *this;
			}
			
			//! Sets the minification and magnification filter modes for the sampler.
			Options& min_mag_filters(vk::Filter min, vk::Filter mag) 
			{ 
				m_min_filter = min; 
				m_mag_filter = mag;
				return *this; 
			}

			//! Sets both the minification and magnification filter modes for the sampler.
			Options& min_mag_filters(vk::Filter filter) { m_min_filter = m_mag_filter = filter; return *this; }

			//! Enables or disables texel anistropic filtering.
			Options& anistropy_enabled(vk::Bool32 anistropy_enabled = VK_TRUE) { m_anistropy_enabled = anistropy_enabled; return *this; }

			//! Sets the anistropy value clamp.
			Options& max_anistropy(float max_anistropy) { m_max_anistropy = max_anistropy; return *this; }
			
			//! Sets the values that are used to clamp the computed level-of-detail value. Note
			//! that the maximum LOD must be greater than or equal to the minimum LOD. 
			Options& lod(float min, float max, float mip_bias) 
			{
				m_min_lod = min; 
				m_max_lod = (max < min) ? min : max;
				m_mip_lod_bias = mip_bias;
				return *this;
			}

			//! Sets the border color used by the sampler.
			Options& border_color(vk::BorderColor border_color = vk::BorderColor::eIntOpaqueWhite) { m_border_color = border_color; return *this; }

			//! Sets the mipmap mode used by the sampler. Possible values are vk::SamplerMipmapMode::eLinear
			//! (default) and vk::SamplerMipmapMode::eNearest.
			Options& mipmap_mode(vk::SamplerMipmapMode mipmap_mode) { m_mipmap_mode = mipmap_mode; return *this; }

			//! By default, the sampler assumes that the application is using normalized texture coordinates.
			//! Calling this function disables this behavior. Note that enabling this features places significant
			//! restrictions on the corresponding image view that will be sampled in the shader. See the spec for 
			//! details.
			Options& enable_unnormalized_coordinates() { m_unnormalized_coordinates = VK_TRUE;  return *this; }

			//! Enables and sets the comparison function that is applied to fetched data before filtering.
			Options& compare_op(vk::CompareOp compare_op) { m_compare_op_enable = VK_TRUE; m_compare_op = compare_op; return *this; }

		private:
			
			vk::SamplerAddressMode m_address_mode_u;
			vk::SamplerAddressMode m_address_mode_v;
			vk::SamplerAddressMode m_address_mode_w;
			vk::Filter m_min_filter;
			vk::Filter m_mag_filter;
			float m_min_lod;
			float m_max_lod;
			float m_mip_lod_bias;
			vk::Bool32 m_anistropy_enabled;
			float m_max_anistropy;
			vk::BorderColor m_border_color;
			vk::SamplerMipmapMode m_mipmap_mode;
			vk::Bool32 m_unnormalized_coordinates;
			vk::Bool32 m_compare_op_enable;
			vk::CompareOp m_compare_op;

			friend class Sampler;
		};

		//! Factory method for returning a new SamplerRef.
		static SamplerRef create(const DeviceRef& device,const Options& options = Options())
		{
			return std::make_shared<Sampler>(device, options);
		}

		Sampler(const DeviceRef& device, const Options& options = Options());
		~Sampler();

		inline vk::Sampler get_handle() const { return m_sampler_handle; }

	private:

		DeviceRef m_device;
		vk::Sampler m_sampler_handle;
	};

	class Image;
	using ImageRef = std::shared_ptr<Image>;
	using ImageRefWeak = std::weak_ptr<Image>;

	class Image : public Noncopyable
	{
	public:

		static const vk::ImageUsageFlags IMAGE_USAGE_ALL;

		//! Factory method for returning a new ImageRef whose device local memory store will be empty.
		static ImageRef create(const DeviceRef& device, 
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
		static ImageRef create(const DeviceRef& device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mip_levels, const std::vector<T>& data)
		{
			return std::make_shared<Image>(device, image_type, image_usage_flags, format, width, height, depth, mip_levels, data);
		}

		//! Factory method for returning a new ImageRef from an LDR image file.
		static ImageRef create(const DeviceRef& device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, const ImageResource& resource)
		{
			return std::make_shared<Image>(device, image_type, image_usage_flags, format, resource);
		}

		//! Factory method for returning a new ImageRef from an HDR image file.
		static ImageRef create(const DeviceRef& device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, const ImageResourceHDR& resource)
		{
			return std::make_shared<Image>(device, image_type, image_usage_flags, format, resource);
		}

		Image(const DeviceRef& device, 
			  vk::ImageType image_type, 
			  vk::ImageUsageFlags image_usage_flags,
			  vk::Format format, 
			  vk::Extent3D dimensions,
			  uint32_t mip_levels = 1, 
			  vk::ImageTiling image_tiling = vk::ImageTiling::eLinear, 
			  uint32_t sample_count = 1);

		template<typename T>
		Image(const DeviceRef& device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, vk::Extent3D dimensions, uint32_t mip_levels, const std::vector<T>& pixels) :
			m_device(device),
			m_image_type(image_type),
			m_image_usage_flags(image_usage_flags),
			m_format(format),
			m_dimensions(dimensions),
			m_mip_levels(mip_levels),
			m_is_array(false)
		{
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

			m_image_handle = m_device->get_handle().createImage(image_create_info);

			initialize_device_memory_with_flags(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

			// Fill the image with the provided data.
			void* mapped_ptr = m_device_memory->map(0, m_device_memory->get_allocation_size());		// Map

			vk::ImageSubresource image_subresource;
			image_subresource.aspectMask = utils::format_to_aspect_mask(m_format);
			image_subresource.arrayLayer = 0;
			image_subresource.mipLevel = 0;

			vk::SubresourceLayout subresource_layout = m_device->get_handle().getImageSubresourceLayout(m_image_handle, image_subresource);

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

		Image(const DeviceRef& device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, const ImageResource& resource) :
			Image(device, image_type, image_usage_flags, format, { resource.width, resource.height, 1 }, 1, resource.contents) {}

		Image(const DeviceRef& device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, const ImageResourceHDR& resource) :
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
																		  uint32_t base_level,
																		  uint32_t level_count,
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

		inline vk::Image get_handle() const { return m_image_handle; }
		inline vk::Format get_format() const { return m_format; }
		inline vk::ImageLayout get_current_layout() const { return m_current_layout; }

	private:

		//! Given the memory requirements of this image, allocate the appropriate type and size of device memory.
		void initialize_device_memory_with_flags(vk::MemoryPropertyFlags memory_property_flags);
		vk::ImageViewType image_view_type_from_parent() const;

		DeviceRef m_device;
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

	class ImageView : public Noncopyable
	{
	public:

		//! Factory method for returning a new ImageViewRef from a parent ImageRef.
		static ImageViewRef create(const DeviceRef& device, const ImageRefWeak& image, uint32_t base_array_layer = 0, uint32_t layer_count = 1, uint32_t base_mip_level = 0, uint32_t level_count = 1)
		{
			vk::ImageSubresourceRange subresource_range = {
				utils::format_to_aspect_mask(image.lock()->m_format),
				base_mip_level, 
				level_count,
				base_array_layer, 
				layer_count
			};

			return std::make_shared<ImageView>(device, image, subresource_range);
		}

		static ImageViewRef create(const DeviceRef& device, const ImageRefWeak& image, const vk::ImageSubresourceRange& subresource_range)
		{
			return std::make_shared<ImageView>(device, image, subresource_range);
		}

		ImageView(const DeviceRef& device, const ImageRefWeak& image, uint32_t base_array_layer = 0, uint32_t layer_count = 1, uint32_t base_mip_level = 0, uint32_t level_count = 1);
		ImageView(const DeviceRef& device, const ImageRefWeak& image, const vk::ImageSubresourceRange& subresource_range);
		~ImageView();

		inline vk::DescriptorImageInfo build_descriptor_info(const SamplerRef& sampler, vk::ImageLayout image_layout = vk::ImageLayout::eShaderReadOnlyOptimal) const
		{
			return{ sampler->get_handle(), m_image_view_handle, image_layout };
		}

		inline vk::ImageView get_handle() const { return m_image_view_handle; }
		inline bool is_valid() const { return !m_image.expired(); }

	private:

		DeviceRef m_device;
		ImageRefWeak m_image;
		vk::ImageSubresourceRange m_subresource_range;
		vk::ImageView m_image_view_handle;
	};

} // namespace graphics
