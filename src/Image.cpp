#include "Image.h"

namespace graphics
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

	Image::Image(const DeviceRef &tDevice, vk::ImageUsageFlags tImageUsageFlags, const ImageResource &tResource, const Options &tOptions) :
		mDevice(tDevice),
		mImageUsageFlags(tImageUsageFlags),
		mFormat(tOptions.mFormat),
		mWidth(tResource.width),
		mHeight(tResource.height),
		mChannels(tResource.channels)
	{
		VkDeviceSize imageSize = mWidth * mHeight * 4;
		
		vk::ImageCreateInfo imageCreateInfo;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.extent.width = mWidth;
		imageCreateInfo.extent.height = mHeight;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.format = mFormat;
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
			memcpy(mappedPtr, tResource.contents.data(), static_cast<size_t>(imageSize));
		}
		else 
		{
			uint8_t* dataAsBytes = reinterpret_cast<uint8_t*>(mappedPtr);
			for (size_t i = 0; i < mHeight; ++i)
			{
				memcpy(&dataAsBytes[i * subresourceLayout.rowPitch],
					   &tResource.contents[i * mWidth * 4],
						mWidth * 4);
			}
		}
			
		mDeviceMemory->unmap();															// Unmap
		
		// Associate the device memory with this buffer object.
		mDevice->getHandle().bindImageMemory(mImageHandle, mDeviceMemory->getHandle(), 0);

		// Create a basic image view for this image.
		vk::ImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.format = mFormat;
		imageViewCreateInfo.image = mImageHandle;
		imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.viewType = vk::ImageViewType::e2D;

		mImageViewHandle = mDevice->getHandle().createImageView(imageViewCreateInfo);

		// Create a basic sampler for this image.
		vk::SamplerCreateInfo samplerCreateInfo;
		samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
		samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
		samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
		samplerCreateInfo.anisotropyEnable = VK_TRUE; 
		samplerCreateInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
		samplerCreateInfo.compareEnable = VK_FALSE;
		samplerCreateInfo.compareOp = vk::CompareOp::eAlways;
		samplerCreateInfo.magFilter = vk::Filter::eLinear;
		samplerCreateInfo.maxAnisotropy = 16;
		samplerCreateInfo.maxLod = 0.0f;
		samplerCreateInfo.minFilter = vk::Filter::eLinear;
		samplerCreateInfo.minLod = 0.0f;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

		mSamplerHandle = mDevice->getHandle().createSampler(samplerCreateInfo);
	}

	Image::~Image()
	{
		mDevice->getHandle().destroyImage(mImageHandle);
	}

} // namespace graphics