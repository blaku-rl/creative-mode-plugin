#pragma once
#include <nlohmann/json.hpp>

struct Settings {
	static const int CURRENT_VERSION = 1;
	int version = 0;
	bool loggingAllowed;
	std::string mapsFolder;

	void SetDefaultValues(const std::filesystem::path& defaultFolderPath);
	bool IsCurrentVersion() const;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Settings, version, loggingAllowed, mapsFolder)