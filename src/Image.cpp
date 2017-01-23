#include "Image.h"

namespace vksp
{

	/*static void loadFile(const std::string &tFilePath)
	{
		int width;
		int height;
		int channels;
		stbi_uc* pixels = stbi_load(tFilePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		
		VkDeviceSize imageSize = width * height * 4;

		if (!pixels) 
		{
			throw std::runtime_error("Failed to load image file");
		}
	}*/

	Image::Options::Options()
	{
		mFormat = vk::Format::eR8G8B8A8Unorm;
		mImageTiling = vk::ImageTiling::eLinear;
	}

	Image::Image(const DeviceRef &tDevice, uint32_t tWidth, uint32_t tHeight, size_t tSize, const void *tData, const Options &tOptions) :
		mDevice(tDevice),
		mWidth(tWidth),
		mHeight(tHeight)
	{
		vk::ImageCreateInfo imageCreateInfo;
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
		imageCreateInfo.usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled;

		mImageHandle = mDevice->getHandle().createImage(imageCreateInfo);
	}

} // namespace vksp