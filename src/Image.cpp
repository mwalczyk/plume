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

#include "Image.h"

namespace graphics
{

	bool ImageBase::isDepthFormat(vk::Format tFormat)
	{
		return (tFormat == vk::Format::eD16Unorm ||
			tFormat == vk::Format::eD16UnormS8Uint ||
			tFormat == vk::Format::eD24UnormS8Uint ||
			tFormat == vk::Format::eD32Sfloat ||
			tFormat == vk::Format::eD32SfloatS8Uint);
	}

	bool ImageBase::isStencilFormat(vk::Format tFormat)
	{
		return (tFormat == vk::Format::eD16UnormS8Uint ||
				tFormat == vk::Format::eD24UnormS8Uint ||
				tFormat == vk::Format::eD32SfloatS8Uint);
	}

	vk::ImageAspectFlags ImageBase::formatToAspectMask(vk::Format tFormat)
	{
		vk::ImageAspectFlags imageAspectFlags;

		if (isDepthFormat(tFormat))
		{
			imageAspectFlags = vk::ImageAspectFlagBits::eDepth;
			if (isStencilFormat(tFormat))
			{
				imageAspectFlags |= vk::ImageAspectFlagBits::eStencil;
			}
		}
		else
		{
			imageAspectFlags = vk::ImageAspectFlagBits::eColor;
		}
		
		return imageAspectFlags;
	}

	ImageBase::~ImageBase()
	{
		mDevice->getHandle().destroyImage(mImageHandle);
	}

	vk::Sampler ImageBase::buildSampler() const
	{
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

		return mDevice->getHandle().createSampler(samplerCreateInfo);
	}

	void ImageBase::initializeDeviceMemoryWithFlags(vk::MemoryPropertyFlags tMemoryPropertyFlags)
	{
		// Retrieve the memory requirements for this image.
		auto memoryRequirements = mDevice->getHandle().getImageMemoryRequirements(mImageHandle);
		auto requiredMemoryProperties = tMemoryPropertyFlags;

		// Allocate device memory.
		mDeviceMemory = DeviceMemory::create(mDevice, memoryRequirements, requiredMemoryProperties);

		// Associate the device memory with this image.
		mDevice->getHandle().bindImageMemory(mImageHandle, mDeviceMemory->getHandle(), 0);
	}

	Image2D::Options::Options()
	{
		mImageTiling = vk::ImageTiling::eLinear;
		mSampleCountFlagBits = vk::SampleCountFlagBits::e1;
		mMipLevels = 1;
	}

	Image2D::Image2D(const DeviceRef &tDevice, vk::ImageUsageFlags tImageUsageFlags, vk::Format tFormat, uint32_t tWidth, uint32_t tHeight, const Options &tOptions) :
		ImageBase(tDevice, tImageUsageFlags, tFormat, tWidth, tHeight, 1),
		mMipLevels(tOptions.mMipLevels)
	{
		mCurrentLayout = vk::ImageLayout::eUndefined;

		vk::ImageCreateInfo imageCreateInfo;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.extent.width = mWidth;
		imageCreateInfo.extent.height = mHeight;
		imageCreateInfo.extent.depth = mDepth;
		imageCreateInfo.format = mFormat;
		imageCreateInfo.initialLayout = mCurrentLayout;
		imageCreateInfo.imageType = vk::ImageType::e2D;
		imageCreateInfo.mipLevels = mMipLevels;
		imageCreateInfo.pQueueFamilyIndices = tOptions.mQueueFamilyIndices.data();
		imageCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(tOptions.mQueueFamilyIndices.size());
		imageCreateInfo.samples = tOptions.mSampleCountFlagBits;
		imageCreateInfo.sharingMode = (imageCreateInfo.pQueueFamilyIndices) ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
		imageCreateInfo.tiling = tOptions.mImageTiling;
		imageCreateInfo.usage = mImageUsageFlags;

		mImageHandle = mDevice->getHandle().createImage(imageCreateInfo);

		initializeDeviceMemoryWithFlags(vk::MemoryPropertyFlagBits::eDeviceLocal);
	}

	Image2D::Image2D(const DeviceRef &tDevice, vk::ImageUsageFlags tImageUsageFlags, vk::Format tFormat, const ImageResource &tResource, const Options &tOptions) :
		ImageBase(tDevice, tImageUsageFlags, tFormat, tResource.width, tResource.height, 1),
		mMipLevels(tOptions.mMipLevels)
	{	
		mCurrentLayout = vk::ImageLayout::ePreinitialized;

		vk::ImageCreateInfo imageCreateInfo;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.extent.width = mWidth;
		imageCreateInfo.extent.height = mHeight;
		imageCreateInfo.extent.depth = mDepth;
		imageCreateInfo.format = mFormat;
		imageCreateInfo.initialLayout = mCurrentLayout;
		imageCreateInfo.imageType = vk::ImageType::e2D;
		imageCreateInfo.mipLevels = mMipLevels;
		imageCreateInfo.pQueueFamilyIndices = tOptions.mQueueFamilyIndices.data();
		imageCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(tOptions.mQueueFamilyIndices.size());
		imageCreateInfo.samples = tOptions.mSampleCountFlagBits;
		imageCreateInfo.sharingMode = (imageCreateInfo.pQueueFamilyIndices) ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;
		imageCreateInfo.tiling = tOptions.mImageTiling;
		imageCreateInfo.usage = mImageUsageFlags; 

		mImageHandle = mDevice->getHandle().createImage(imageCreateInfo);
		
		initializeDeviceMemoryWithFlags(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	
		// Fill the image with the provided data.
		void* mappedPtr = mDeviceMemory->map(0, mDeviceMemory->getAllocationSize());	// Map

		vk::ImageSubresource imageSubresource;
		imageSubresource.aspectMask = formatToAspectMask(mFormat);
		imageSubresource.arrayLayer = 0;
		imageSubresource.mipLevel = 0;

		vk::SubresourceLayout subresourceLayout = mDevice->getHandle().getImageSubresourceLayout(mImageHandle, imageSubresource);
			
		// The subresource has no additional padding, so we can directly copy the pixel data into the image.
		// This usually happens when the requested image is a power-of-two texture.
		VkDeviceSize imageSize = mWidth * mHeight * 4;
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
	}

	vk::ImageView Image2D::buildImageView() const
	{
		vk::ImageViewCreateInfo imageViewCreateInfo;
		imageViewCreateInfo.format = mFormat;
		imageViewCreateInfo.image = mImageHandle;
		imageViewCreateInfo.subresourceRange.aspectMask = formatToAspectMask(mFormat);
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
		
		return mDevice->getHandle().createImageView(imageViewCreateInfo);
	}

} // namespace graphics