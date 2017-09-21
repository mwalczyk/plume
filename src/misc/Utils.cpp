#include "Utils.h"

namespace graphics
{

	namespace utils
	{

		bool is_depth_format(vk::Format format)
		{
			return (format == vk::Format::eD16Unorm ||
				    format == vk::Format::eD16UnormS8Uint ||
					format == vk::Format::eD24UnormS8Uint ||
					format == vk::Format::eD32Sfloat ||
					format == vk::Format::eD32SfloatS8Uint);
		}

		bool is_stencil_format(vk::Format format)
		{
			return (format == vk::Format::eD16UnormS8Uint ||
				    format == vk::Format::eD24UnormS8Uint ||
				    format == vk::Format::eD32SfloatS8Uint);
		}

		vk::ImageAspectFlags format_to_aspect_mask(vk::Format format)
		{
			vk::ImageAspectFlags image_aspect_flags;

			if (is_depth_format(format))
			{
				image_aspect_flags = vk::ImageAspectFlagBits::eDepth;
				if (is_stencil_format(format))
				{
					image_aspect_flags |= vk::ImageAspectFlagBits::eStencil;
				}
			}
			else
			{
				image_aspect_flags = vk::ImageAspectFlagBits::eColor;
			}

			return image_aspect_flags;
		}

		vk::SampleCountFlagBits sample_count_to_flags(uint32_t count)
		{
			switch (count)
			{
			case 1:
				return vk::SampleCountFlagBits::e1;
			case 2:
				return vk::SampleCountFlagBits::e2;
			case 4: 
				return vk::SampleCountFlagBits::e4;
			case 8:
				return vk::SampleCountFlagBits::e8;
			case 16:
				return vk::SampleCountFlagBits::e16;
			case 32:
				return vk::SampleCountFlagBits::e32;
			case 64:
				return vk::SampleCountFlagBits::e64;
			default:
				std::cout << "Warning - the sample count passed to `sample_count_to_flags` was invalid: returning vk::SampleCountFlagBits::e1 \n";
				return vk::SampleCountFlagBits::e1;
			}
		}

	} // namespace utils

} // namespace graphics