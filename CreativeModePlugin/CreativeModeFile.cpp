#include "pch.h"
#include "CreativeModeFile.h"
#include <format>
#include <fstream>

CreativeModeFile::CreativeModeFile(const std::filesystem::directory_entry& mapEntry)
{
	this->mapName = mapEntry.path().stem().string();
	this->lastModified = std::filesystem::last_write_time(mapEntry);
	GetAuthorName(mapEntry);
}

std::string CreativeModeFile::GetMapString() const
{
	std::stringstream ss;
	const auto systemTime = std::chrono::clock_cast<std::chrono::system_clock>(this->lastModified);
	const auto time = std::chrono::system_clock::to_time_t(systemTime);
	auto tm = *std::localtime(&time);
	ss << this->mapName << "," << this->mapAuthor << "," << std::put_time(&tm, "%Y/%m/%d");
	return ss.str();
}

void CreativeModeFile::GetAuthorName(const std::filesystem::directory_entry& mapEntry)
{
	this->mapAuthor = "Author not read";
	std::string authorTag = "-MapCreatorInfo ";
	std::string pluginTag = "-MapSaveVersion ";
	std::string oldTag = "MapSaveVersion";

	std::ifstream mapFile(mapEntry.path());
	if (mapFile.is_open()) {
		bool authorFound = false;
		std::string line;
		std::getline(mapFile, line);

		if (line.starts_with(pluginTag)) {
			std::getline(mapFile, line);
			this->mapAuthor = line.substr(authorTag.size());
		}
		else if (line.starts_with(oldTag)) {
			std::getline(mapFile, line);
			this->mapAuthor = line.substr(oldTag.size());
		}
		else {
			this->mapAuthor = line;
		}
	}
	mapFile.close();
}
