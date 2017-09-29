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

namespace plume
{

	namespace fsys
	{

		std::string ResourceManager::default_path = "../assets/";

		FileResource ResourceManager::load_file(const std::string& file_name)
		{
			std::string path_to = default_path + file_name;

			// Start reading at the end of the file to determine file size.
			std::ifstream file(path_to, std::ios::ate | std::ios::binary);

			if (!file.is_open())
			{
				throw std::runtime_error("Failed to load file: " + path_to);
			}

			// After recording the file size, go back to the beginning of the file.
			size_t total_size = static_cast<size_t>(file.tellg());
			file.seekg(0);

			// Read and close the file.
			FileResource resource = { std::vector<uint8_t>(total_size) };
			auto data = reinterpret_cast<char*>(resource.contents.data());
			file.read(data, total_size);
			file.close();

			return resource;
		}

		ImageResource ResourceManager::load_image(const std::string& file_name, bool force_channels)
		{
			std::string path_to = default_path + file_name;

			ImageResource resource;

			// Read the image contents.
			stbi_uc* pixels = stbi_load(path_to.c_str(), (int*)(&resource.width), (int*)(&resource.height), (int*)(&resource.channels), STBI_rgb_alpha);
			if (!pixels)
			{
				throw std::runtime_error("Failed to load image: " + path_to);
			}
			if (force_channels)
			{
				resource.channels = 4;
			}
			resource.contents = std::vector<uint8_t>(pixels, pixels + resource.width * resource.height * resource.channels);

			stbi_image_free(pixels);

			return resource;
		}

		ImageResourceHDR ResourceManager::load_image_hdr(const std::string& file_name, bool force_channels)
		{
			std::string path_to = default_path + file_name;

			ImageResourceHDR resource;

			// Read the image contents.
			float* pixels = stbi_loadf(path_to.c_str(), (int*)(&resource.width), (int*)(&resource.height), (int*)(&resource.channels), STBI_rgb_alpha);
			if (!pixels)
			{
				throw std::runtime_error("Failed to load image: " + path_to);
			}
			if (force_channels)
			{
				resource.channels = 4;
			}
			resource.contents = std::vector<float>(pixels, pixels + resource.width * resource.height * resource.channels);

			stbi_image_free(pixels);

			return resource;
		}

	} // namespace fsys

} // namespace plume