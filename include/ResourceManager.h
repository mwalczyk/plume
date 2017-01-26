#pragma once

#include <vector>
#include <fstream>
#include <string>

struct FileResource
{
	std::vector<uint8_t> contents;
};

struct ImageResource
{
	uint32_t width;
	uint32_t height;
	uint32_t channels;
	std::vector<uint8_t> contents;
};

class ResourceManager
{
public:

	static ResourceManager& resourceManager()
	{
		static ResourceManager manager;
		return manager;
	}

	static FileResource loadFile(const std::string &tFileName);
	static ImageResource loadImage(const std::string &tFileName, bool tForceChannels = true);

	ResourceManager(const ResourceManager &tOther) = delete;
	ResourceManager& operator=(const ResourceManager &tOther) = delete;

private:

	ResourceManager() = default;
};