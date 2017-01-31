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

#include "ResourceManager.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

FileResource ResourceManager::loadFile(const std::string &tFileName)
{
	// Start reading at the end of the file to determine file size.
	std::ifstream file(tFileName, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		throw std::runtime_error("Failed to load file: " + tFileName);
	}

	// After recording the file size, go back to the beginning of the file.
	size_t fileSize = static_cast<size_t>(file.tellg());
	file.seekg(0);

	// Read and close the file.
	FileResource resource = { std::vector<uint8_t>(fileSize) };
	auto data = reinterpret_cast<char*>(resource.contents.data());
	file.read(data, fileSize);
	file.close();

	return resource;
}

ImageResource ResourceManager::loadImage(const std::string &tFileName, bool tForceChannels)
{
	ImageResource resource;

	// Read the image contents.
	stbi_uc* pixels = stbi_load(tFileName.c_str(), (int*)(&resource.width), (int*)(&resource.height), (int*)(&resource.channels), STBI_rgb_alpha);
	if (!pixels)
	{
		throw std::runtime_error("Failed to load image: " + tFileName);
	}
	if (tForceChannels)
	{
		resource.channels = 4;
	}
	resource.contents = std::vector<uint8_t>(pixels, pixels + resource.width * resource.height * resource.channels);

	stbi_image_free(pixels);

	return resource;
}