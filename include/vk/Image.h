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

	class Image : public Noncopyable
	{
	public:

		class Options
		{
		public:
			Options();

			Options& image_tiling(vk::ImageTiling image_tiling) { m_image_tiling = image_tiling; return *this; }
			Options& sample_count(vk::SampleCountFlagBits sample_count_flag_bits) { m_sample_count_flag_bits = sample_count_flag_bits; return *this; }
			Options& queue_family_indices(const std::vector<uint32_t>& queue_family_indices) { m_queue_family_indices = queue_family_indices; return *this; }
			Options& mip_levels(uint32_t mip_levels) { m_mip_levels = mip_levels; return *this; }

		private:

			vk::ImageTiling m_image_tiling;
			vk::SampleCountFlagBits m_sample_count_flag_bits;
			std::vector<uint32_t> m_queue_family_indices;
			uint32_t m_mip_levels;

			friend class Image;
		};

		//! Factory method for returning a new ImageRef that will be empty.
		static ImageRef create(const DeviceRef& device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, uint32_t width, uint32_t height, uint32_t depth, const Options& options = Options())
		{
			return std::make_shared<Image>(device, image_type, image_usage_flags, format, width, height, depth, options);
		}

		//! Factory method for returning a new ImageRef that will be pre-initialized with the user supplied data.
		template<class T>
		static ImageRef create(const DeviceRef& device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, uint32_t width, uint32_t height, uint32_t depth, const std::vector<T>& data, const Options& options = Options())
		{
			return std::make_shared<Image>(device, image_type, image_usage_flags, format, width, height, depth, sizeof(T) * data.size(), data.data(), options);
		}

		//! Factory method for returning a new ImageRef from an image file.
		static ImageRef create(const DeviceRef& device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, const ImageResource& resource, const Options& options = Options())
		{
			return std::make_shared<Image>(device, image_type, image_usage_flags, format, resource, options);
		}

		Image(const DeviceRef& device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, uint32_t width, uint32_t height, uint32_t depth, const Options& options = Options());
		Image(const DeviceRef& device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, uint32_t width, uint32_t height, uint32_t depth, size_t size, const void* data, const Options& options = Options());
		Image(const DeviceRef& device, vk::ImageType image_type, vk::ImageUsageFlags image_usage_flags, vk::Format format, const ImageResource& resource, const Options& options = Options());

		~Image();

		vk::ImageView build_image_view() const;

		inline vk::Image get_handle() const { return m_image_handle; }
		inline vk::Format get_format() const { return m_format; }
	    inline vk::DescriptorImageInfo build_descriptor_info(const SamplerRef& sampler, vk::ImageView image_view, vk::ImageLayout image_layout = vk::ImageLayout::eShaderReadOnlyOptimal) const { return { sampler->get_handle(), image_view, image_layout }; }
		
	protected:

		//! Given the memory requirements of this image, allocate the appropriate type and size of device memory.
		void initialize_device_memory_with_flags(vk::MemoryPropertyFlags memory_property_flags);
		
		DeviceRef m_device;
		DeviceMemoryRef m_device_memory;
		vk::Image m_image_handle;
		vk::ImageType m_image_type;
		vk::ImageUsageFlags m_image_usage_flags;
		vk::Format m_format;
		vk::ImageLayout m_current_layout;
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_depth;
		uint32_t m_channels;
		uint32_t m_mip_levels;

		friend class CommandBuffer;
	};

} // namespace graphics
