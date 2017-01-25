#include "Image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace vksp
{

	Image::Options::Options()
	{
		mFormat = vk::Format::eR8G8B8A8Unorm;
		mImageTiling = vk::ImageTiling::eLinear;
	}

	Image::Image(const DeviceRef &tDevice, vk::ImageUsageFlags tImageUsageFlags, uint32_t tWidth, uint32_t tHeight, size_t tSize, const void *tData, const Options &tOptions) :
		mDevice(tDevice),
		mImageUsageFlags(tImageUsageFlags),
		mWidth(tWidth),
		mHeight(tHeight)
	{
		/*vk::ImageCreateInfo imageCreateInfo;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.extent.width = mWidth;
		imageCreateInfo.extent.height = mHeight;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.format = tOptions.mFormat;
		imageCreateInfo.initialLayout = (tData) ? vk::ImageLayout::ePreinitialized : vk::ImageLayout::eUndefined;
		imageCreateInfo.imageType = vk::ImageType::e2D;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.pQueueFamilyIndices = nullptr;
		imageCreateInfo.queueFamilyIndexCount = 0;
		imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
		imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
		imageCreateInfo.tiling = tOptions.mImageTiling;
		imageCreateInfo.usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled;*/
	}

	Image::Image(const DeviceRef &tDevice, vk::ImageUsageFlags tImageUsageFlags, const std::string &tFilePath, const Options &tOptions) :
		mDevice(tDevice),
		mImageUsageFlags(tImageUsageFlags)
	{
		stbi_uc* pixels = stbi_load(tFilePath.c_str(), (int*)(&mWidth), (int*)(&mHeight), (int*)(&mChannels), STBI_rgb_alpha);
		VkDeviceSize imageSize = mWidth * mHeight * 4;
		
		if (!pixels)
		{
			throw std::runtime_error("Failed to load image file");
		}

		vk::ImageCreateInfo imageCreateInfo;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.extent.width = mWidth;
		imageCreateInfo.extent.height = mHeight;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.format = tOptions.mFormat;
		imageCreateInfo.initialLayout = vk::ImageLayout::ePreinitialized;
		imageCreateInfo.imageType = vk::ImageType::e2D;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.pQueueFamilyIndices = nullptr;
		imageCreateInfo.queueFamilyIndexCount = 0;
		imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
		imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
		imageCreateInfo.tiling = tOptions.mImageTiling;
		imageCreateInfo.usage = mImageUsageFlags; 

		mImageHandle = mDevice->getHandle().createImage(imageCreateInfo);

		// Store the memory requirements for this buffer object.
		auto memoryRequirements = mDevice->getHandle().getImageMemoryRequirements(mImageHandle);
		auto requiredMemoryProperties = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

		// Allocate device memory.
		mDeviceMemory = DeviceMemory::create(mDevice, memoryRequirements, requiredMemoryProperties);

		// Fill the image with the provided data.
		void* mappedPtr = mDeviceMemory->map(0, mDeviceMemory->getAllocationSize());	// Map

		vk::ImageSubresource imageSubresource;
		imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageSubresource.arrayLayer = 0;
		imageSubresource.mipLevel = 0;

		vk::SubresourceLayout subresourceLayout = mDevice->getHandle().getImageSubresourceLayout(mImageHandle, imageSubresource);
			
		// The subresource has no additional padding, so we can directly copy the pixel data into the image.
		// This usually happens when the requested image is a power-of-two texture.
		if (subresourceLayout.rowPitch == mWidth * 4)
		{
			memcpy(mappedPtr, pixels, static_cast<size_t>(imageSize));
		}
		else 
		{
			uint8_t* dataAsBytes = reinterpret_cast<uint8_t*>(mappedPtr);
			for (size_t i = 0; i < mHeight; ++i)
			{
				memcpy(&dataAsBytes[i * subresourceLayout.rowPitch],
					   &pixels[i * mWidth * 4],
						mWidth * 4);
			}
		}
			
		mDeviceMemory->unmap();															// Unmap
		
		// Associate the device memory with this buffer object.
		mDevice->getHandle().bindImageMemory(mImageHandle, mDeviceMemory->getHandle(), 0);

		stbi_image_free(pixels);
	}

	Image::~Image()
	{
		mDevice->getHandle().destroyImage(mImageHandle);
	}

} // namespace vksp