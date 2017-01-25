#pragma once

#include <memory>
#include <vector>
#include <string>

#include "Platform.h"
#include "Device.h"
#include "DeviceMemory.h"

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
		static ImageRef create(const DeviceRef &tDevice, vk::ImageUsageFlags tImageUsageFlags, uint32_t tWidth, uint32_t tHeight, const std::vector<T> &tData, const Options &tOptions = Options())
		{
			return std::make_shared<Image>(tDevice, tImageUsageFlags, tWidth, tHeight, sizeof(T) * tData.size(), tData.data(), tOptions);
		}

		//! Factory method for returning a new ImageRef from an image file.
		static ImageRef create(const DeviceRef &tDevice, vk::ImageUsageFlags tImageUsageFlags, const std::string &tFilePath, const Options &tOptions = Options())
		{
			return std::make_shared<Image>(tDevice, tImageUsageFlags, tFilePath, tOptions);
		}

		Image(const DeviceRef &tDevice, vk::ImageUsageFlags tImageUsageFlags, uint32_t tWidth, uint32_t tHeight, size_t tSize, const void *tData, const Options &tOptions = Options());
		Image(const DeviceRef &tDevice, vk::ImageUsageFlags tImageUsageFlags, const std::string &tFilePath, const Options &tOptions = Options());
		~Image();

		inline vk::Image getHandle() const { return mImageHandle; }
		inline vk::DeviceMemory getDeviceMemoryHandle() const { return mDeviceMemoryHandle; }

	private:
	
		DeviceRef mDevice;	
		DeviceMemoryRef mDeviceMemory;
		vk::Image mImageHandle;
		vk::DeviceMemory mDeviceMemoryHandle;
		vk::ImageUsageFlags mImageUsageFlags;
		uint32_t mWidth;
		uint32_t mHeight;
		uint32_t mChannels;
	};

} // namespace vksp
