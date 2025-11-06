#pragma once
#include <string>
#include <filesystem>

struct CreativeModeFile {
	std::string mapName;
	std::string mapAuthor;
	std::filesystem::file_time_type lastModified;

	CreativeModeFile(const std::filesystem::directory_entry& mapFile);
	std::string GetMapString() const;

private:
	void GetAuthorName(const std::filesystem::directory_entry& mapEntry);
};