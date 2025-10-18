#include "pch.h"
#include "Settings.h"

void Settings::SetDefaultValues(const std::filesystem::path& defaultFolderPath)
{
	if (version >= CURRENT_VERSION) {
		version = CURRENT_VERSION;
		return;
	}

	version = CURRENT_VERSION;
	mapsFolder = defaultFolderPath.string();
	loggingAllowed = true;
}

bool Settings::IsCurrentVersion() const
{
	return version == CURRENT_VERSION;
}