#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include <bakkesmod/wrappers/kismet/SequenceVariableWrapper.h>
#include "CreativeModeFile.h"
#include "Position.h"
#include "Settings.h"

#include "version.h"
constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

class CreativeModePlugin: public BakkesMod::Plugin::BakkesModPlugin, public SettingsWindowBase
	//,public PluginWindowBase // Uncomment if you want to render your own plugin window
{
	std::map<std::string, SequenceVariableWrapper> mapVariables;
	std::map<std::string, std::function<void()>> commands;
	std::queue<std::tuple<std::string, Position>> objectsToLoad;
	std::filesystem::path creativeModeFolder = gameWrapper->GetDataFolder() / "CreativeMode";
	std::string unusedList = "ObjectList_Used_All";
	std::string objectListPrefix = "ObjectList_Used_";
	std::string availListPrefix = "ObjectList_Available_";
	std::string fileExtension = ".data";
	std::filesystem::path settingsFile = creativeModeFolder / "Settings.json";
	Settings pluginSettings;
	
	void onLoad() override;
	void OnMapLoad(std::string eventName);
	void OnPhysicsTick(std::string eventName);
	std::vector<CreativeModeFile> GetAllMaps() const;
	std::string GetKismetStringValue(const std::string& name);
	void SetLevelListInfoVariable();
	void PlaceObject();
	void LoadMap();
	void SaveMap();
	bool IsMapFolderValid() const;
	bool LoadSettings();
	void SaveSettings() const;

public:
	void RenderSettings() override;
	//void RenderWindow() override; // Uncomment if you want to render your own plugin window
};
