#pragma once

#include <memory>
#include <vector>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

#include "Platform.h"
#include "Device.h"

namespace vksp
{

	class Image;
	using ImageRef = std::shared_ptr<Image>;

	class Image
	{
	public:

		struct Options
		{
			Options();

			Options& format(vk::Format tFormat) { mFormat = tFormat; return *this; }
			Options& imageTiling(vk::ImageTiling tImageTiling) { mImageTiling = tImageTiling; return *this; }

			vk::Format mFormat;
			vk::ImageTiling mImageTiling;
		};

		//! Factory method for returning a new ImageRef.
		template<class T>
		static ImageRef create(const DeviceRef &tDevice, uint32_t tWidth, uint32_t tHeight, const std::vector<T> &tData, const Options &tOptions = Options())
		{
			return std::make_shared<Image>(tDevice, tWidth, tHeight, sizeof(T) * tData.size(), tData.data(), tOptions);
		}

		Image(const DeviceRef &tDevice, uint32_t tWidth, uint32_t tHeight, size_t tSize, const void *tData, const Options &tOptions = Options());
		~Image();

		inline vk::Image getHandle() const { return mImageHandle; }
		inline vk::DeviceMemory getDeviceMemoryHandle() const { return mDeviceMemoryHandle; }

	private:
	


		DeviceRef mDevice;	
		vk::Image mImageHandle;
		vk::DeviceMemory mDeviceMemoryHandle;
		uint32_t mWidth;
		uint32_t mHeight;
	};

} // namespace vksp
