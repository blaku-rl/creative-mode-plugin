#include "pch.h"
#include "CreativeModeFile.h"
#include <format>
#include <fstream>

CreativeModeFile::CreativeModeFile(const std::filesystem::directory_entry& mapEntry)
{
	this->mapName = mapEntry.path().stem().string();
	this->mapAuthor = "Author not read";
	this->lastModified = std::filesystem::last_write_time(mapEntry);

	std::ifstream mapFile(mapEntry.path());
	if (mapFile.is_open())
		std::getline(mapFile, this->mapAuthor);
	mapFile.close();
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
