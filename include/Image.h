#pragma once

#include <memory>
#include <vector>
#include <string>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Platform.h"
#include "Device.h"

namespace vk
{

	class Image;
	using ImageRef = std::shared_ptr<Image>;

	class Image
	{
	public:

		struct Options
		{
			Options();

			Options& format(VkFormat tFormat) { mFormat = tFormat; return *this; }
			Options& imageTiling(VkImageTiling tImageTiling) { mImageTiling = tImageTiling; return *this; }

			VkFormat mFormat;
			VkImageTiling mImageTiling;
		};

		//! Factory method for returning a new ImageRef.
		template<class T>
		static ImageRef create(const DeviceRef &tDevice, uint32_t tWidth, uint32_t tHeight, const std::vector<T> &tData, const Options &tOptions = Options())
		{
			return std::make_shared<Image>(tDevice, tWidth, tHeight, sizeof(T) * tData.size(), tData.data(), tOptions);
		}

		Image(const DeviceRef &tDevice, uint32_t tWidth, uint32_t tHeight, size_t tSize, const void *tData, const Options &tOptions = Options());
		~Image();

		inline VkImage getHandle() const { return mImageHandle; }
		inline VkDeviceMemory getDeviceMemoryHandle() const { return mDeviceMemoryHandle; }

	private:
	
		VkImage mImageHandle;
		VkDeviceMemory mDeviceMemoryHandle;

		DeviceRef mDevice;	

		uint32_t mWidth;
		uint32_t mHeight;
	};

} // namespace vk
