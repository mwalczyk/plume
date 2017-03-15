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

namespace graphics
{

	class ImageBase;
	using ImageBaseRef = std::shared_ptr<ImageBase>;
	using ImageRef = ImageBaseRef;

	class ImageBase : public Noncopyable
	{
	public:
		virtual ~ImageBase();

		//! Determine whether or not an image format contains a depth component.
		static bool is_depth_format(vk::Format format);

		//! Determine whether or not an image format contains a stencil component.
		static bool is_stencil_format(vk::Format format);

		//! Translate an image format into the appropriate aspect mask flags.
		static vk::ImageAspectFlags format_to_aspect_mask(vk::Format format);

		virtual vk::ImageView build_image_view() const = 0;

		//! Image samplers are used by the implementation to read image data and apply filtering and
		//! other transformations inside of a shader. Note that a single sampler can be used with multiple 
		//! attachments when constructing descriptor sets.
		virtual vk::Sampler build_sampler() const final;
		virtual inline vk::Image get_handle() const final { return m_image_handle; }
		virtual inline vk::Format get_format() const final { return m_format; }
		virtual inline vk::DescriptorImageInfo build_descriptor_info(vk::Sampler sampler, vk::ImageView image_view, vk::ImageLayout image_layout = vk::ImageLayout::eShaderReadOnlyOptimal) const { return { sampler, image_view, image_layout }; }
	
	protected:

		ImageBase(const DeviceRef& device, vk::ImageUsageFlags image_usage_flags, vk::Format format, uint32_t width, uint32_t height, uint32_t depth) :
			m_device(device),
			m_image_usage_flags(image_usage_flags),
			m_format(format),
			m_width(width),
			m_height(height),
			m_depth(depth)
		{
		}

		//! Given the memory requirements of this image, allocate the appropriate type and size of device memory.
		void initialize_device_memory_with_flags(vk::MemoryPropertyFlags memory_property_flags);
		
		DeviceRef m_device;
		DeviceMemoryRef m_device_memory;
		vk::Image m_image_handle;
		vk::ImageUsageFlags m_image_usage_flags;
		vk::Format m_format;
		vk::ImageLayout m_current_layout;
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_depth;
		uint32_t m_channels;

		friend class CommandBuffer;
	};

	class Image2D;
	using Image2DRef = std::shared_ptr<Image2D>;

	class Image2D: public ImageBase
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

			friend class Image2D;
		};

		//! Factory method for returning a new ImageRef that will be empty.
		static Image2DRef create(const DeviceRef& device, vk::ImageUsageFlags image_usage_flags, vk::Format format, uint32_t width, uint32_t height, const Options& options = Options())
		{
			return std::make_shared<Image2D>(device, image_usage_flags, format, width, height, options);
		}

		//! Factory method for returning a new ImageRef that will be pre-initialized with data.
		template<class T>
		static Image2DRef create(const DeviceRef& device, vk::ImageUsageFlags image_usage_flags, vk::Format format, uint32_t width, uint32_t height, const std::vector<T>& data, const Options& options = Options())
		{
			return std::make_shared<Image2D>(device, image_usage_flags, format, width, height, sizeof(T) * data.size(), data.data(), options);
		}

		//! Factory method for returning a new ImageRef from an image file.
		static Image2DRef create(const DeviceRef& device, vk::ImageUsageFlags image_usage_flags, vk::Format format, const ImageResource& resource, const Options& options = Options())
		{
			return std::make_shared<Image2D>(device, image_usage_flags, format, resource, options);
		}

		vk::ImageView build_image_view() const override;

		Image2D(const DeviceRef& device, vk::ImageUsageFlags image_usage_flags, vk::Format format, uint32_t width, uint32_t height, const Options& options = Options());
		Image2D(const DeviceRef& device, vk::ImageUsageFlags image_usage_flags, vk::Format format, uint32_t width, uint32_t height, size_t size, const void* data, const Options& options = Options());
		Image2D(const DeviceRef& device, vk::ImageUsageFlags image_usage_flags, vk::Format format, const ImageResource& resource, const Options& options = Options());

	private:

		uint32_t m_mip_levels;
	};

} // namespace graphics
