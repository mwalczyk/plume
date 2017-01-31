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
		static bool isDepthFormat(vk::Format tFormat);

		//! Determine whether or not an image format contains a stencil component.
		static bool isStencilFormat(vk::Format tFormat);

		//! Translate an image format into the appropriate aspect mask flags.
		static vk::ImageAspectFlags formatToAspectMask(vk::Format tFormat);

		virtual vk::ImageView buildImageView() const = 0;

		//! Image samplers are used by the implementation to read image data and apply filtering and
		//! other transformations inside of a shader. Note that a single sampler can be used with multiple 
		//! attachments when constructing descriptor sets.
		virtual vk::Sampler buildSampler() const final;
		virtual inline vk::Image getHandle() const final { return mImageHandle; }
		virtual inline vk::Format getFormat() const final { return mFormat; }
		virtual inline vk::DescriptorImageInfo buildDescriptorInfo(vk::Sampler tSampler, vk::ImageView tImageView, vk::ImageLayout tImageLayout = vk::ImageLayout::eShaderReadOnlyOptimal) const { return { tSampler, tImageView, tImageLayout }; }
	
	protected:

		ImageBase(const DeviceRef &tDevice, vk::ImageUsageFlags tImageUsageFlags, vk::Format tFormat, uint32_t tWidth, uint32_t tHeight, uint32_t tDepth) :
			mDevice(tDevice),
			mImageUsageFlags(tImageUsageFlags),
			mFormat(tFormat),
			mWidth(tWidth),
			mHeight(tHeight),
			mDepth(tDepth)
		{
		}

		//! Given the memory requirements of this image, allocate the appropriate type and size of device memory.
		void initializeDeviceMemoryWithFlags(vk::MemoryPropertyFlags tMemoryPropertyFlags);
		
		DeviceRef mDevice;
		DeviceMemoryRef mDeviceMemory;
		vk::Image mImageHandle;
		vk::ImageUsageFlags mImageUsageFlags;
		vk::Format mFormat;
		vk::ImageLayout mCurrentLayout;
		uint32_t mWidth;
		uint32_t mHeight;
		uint32_t mDepth;
		uint32_t mChannels;

		friend class CommandBuffer;
	};

	class Image2D;
	using Image2DRef = std::shared_ptr<Image2D>;

	class Image2D: public ImageBase
	{
	public:

		struct Options
		{
			Options();

			Options& imageTiling(vk::ImageTiling tImageTiling) { mImageTiling = tImageTiling; return *this; }
			Options& sampleCount(vk::SampleCountFlagBits tSampleCountFlagBits) { mSampleCountFlagBits = tSampleCountFlagBits; return *this; }
			Options& queueFamilyIndices(const std::vector<uint32_t> &tQueueFamilyIndices) { mQueueFamilyIndices = tQueueFamilyIndices; return *this; }
			Options& mipLevels(uint32_t tMipLevels) { mMipLevels = tMipLevels; return *this; }

			vk::ImageTiling mImageTiling;
			vk::SampleCountFlagBits mSampleCountFlagBits;
			std::vector<uint32_t> mQueueFamilyIndices;
			uint32_t mMipLevels;
		};

		//! Factory method for returning a new ImageRef that will be empty.
		static Image2DRef create(const DeviceRef &tDevice, vk::ImageUsageFlags tImageUsageFlags, vk::Format tFormat, uint32_t tWidth, uint32_t tHeight, const Options &tOptions = Options())
		{
			return std::make_shared<Image2D>(tDevice, tImageUsageFlags, tFormat, tWidth, tHeight, tOptions);
		}

		//! Factory method for returning a new ImageRef that will be pre-initialized with data.
		template<class T>
		static Image2DRef create(const DeviceRef &tDevice, vk::ImageUsageFlags tImageUsageFlags, vk::Format tFormat, uint32_t tWidth, uint32_t tHeight, const std::vector<T> &tData, const Options &tOptions = Options())
		{
			return std::make_shared<Image2D>(tDevice, tImageUsageFlags, tFormat, tWidth, tHeight, sizeof(T) * tData.size(), tData.data(), tOptions);
		}

		//! Factory method for returning a new ImageRef from an image file.
		static Image2DRef create(const DeviceRef &tDevice, vk::ImageUsageFlags tImageUsageFlags, vk::Format tFormat, const ImageResource &tResource, const Options &tOptions = Options())
		{
			return std::make_shared<Image2D>(tDevice, tImageUsageFlags, tFormat, tResource, tOptions);
		}

		vk::ImageView buildImageView() const override;

		Image2D(const DeviceRef &tDevice, vk::ImageUsageFlags tImageUsageFlags, vk::Format tFormat, uint32_t tWidth, uint32_t tHeight, const Options &tOptions = Options());
		Image2D(const DeviceRef &tDevice, vk::ImageUsageFlags tImageUsageFlags, vk::Format tFormat, uint32_t tWidth, uint32_t tHeight, size_t tSize, const void *tData, const Options &tOptions = Options());
		Image2D(const DeviceRef &tDevice, vk::ImageUsageFlags tImageUsageFlags, vk::Format tFormat, const ImageResource &tResource, const Options &tOptions = Options());

	private:

		uint32_t mMipLevels;
	};

} // namespace graphics
