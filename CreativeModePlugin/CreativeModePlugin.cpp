#include "pch.h"
#include "CreativeModePlugin.h"
#include <algorithm>
#include <ranges>
#include <fstream>
#include <bakkesmod/wrappers/kismet/SequenceWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceVariableWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceOpWrapper.h>
#include <bakkesmod/wrappers/kismet/SequenceObjectWrapper.h>
#include "Utility.h"
//#include "IMGUI/imgui_internal.h"

BAKKESMOD_PLUGIN(CreativeModePlugin, "Creative Mode Plugin", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;
bool loggingIsAllowed;

void CreativeModePlugin::onLoad()
{
	using namespace std::placeholders;
	_globalCvarManager = cvarManager;

	gameWrapper->HookEventPost("Function Engine.HUD.SetShowScores", std::bind(&CreativeModePlugin::OnMapLoad, this, _1));
	gameWrapper->HookEventPost("Function TAGame.Car_TA.SetVehicleInput", std::bind(&CreativeModePlugin::OnPhysicsTick, this, _1));

	commands["LevelsListInfo"] = std::bind(&CreativeModePlugin::SetLevelListInfoVariable, this);
	commands["load"] = std::bind(&CreativeModePlugin::LoadMap, this);
	commands["save"] = std::bind(&CreativeModePlugin::SaveMap, this);
	commands["nextobject"] = std::bind(&CreativeModePlugin::PlaceObject, this);

	if (!std::filesystem::exists(creativeModeFolder))
		std::filesystem::create_directories(creativeModeFolder);

	if (!LoadSettings()) {
		pluginSettings.SetDefaultValues(creativeModeFolder / "Maps");
		std::filesystem::create_directories(creativeModeFolder / "Maps");
	}

	loggingIsAllowed = pluginSettings.loggingAllowed;

	auto sequence = gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) return;
	sequence.ActivateRemoteEvents("PluginLoaded");
}

void CreativeModePlugin::OnMapLoad(std::string eventName) 
{
	if (!gameWrapper->GetGameEventAsServer()) { return; }
	auto sequence = gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) { return; }
	mapVariables = sequence.GetAllSequenceVariables(false);
}

void CreativeModePlugin::OnPhysicsTick(std::string eventName)
{
	if (!gameWrapper->GetGameEventAsServer() or gameWrapper->IsInOnlineGame()) return;
	auto sequence = gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) return;

	auto pluginControlVar = mapVariables.find("PluginControl");
	if (pluginControlVar != mapVariables.end() and pluginControlVar->second.IsString() and pluginControlVar->second.GetString() != "") {
		auto command = pluginControlVar->second.GetString();
		pluginControlVar->second.SetString("");
		LOG("Attempting to run command {}", command);
		if (commands.find(command) != commands.end())
			commands[command]();
	}
}

std::vector<CreativeModeFile> CreativeModePlugin::GetAllMaps() const
{
	std::vector<CreativeModeFile> maps = {};

	if (!IsMapFolderValid()) {
		LOG("The maps folder '{}' either doesn't exist or isn't a folder", pluginSettings.mapsFolder);
		return maps;
	}
	
	for (auto const& file : std::filesystem::directory_iterator(pluginSettings.mapsFolder)) {
		if (!file.is_regular_file() or !file.path().has_extension() or file.path().extension().string() != fileExtension) continue;
		maps.emplace_back(file);
	}

	std::ranges::sort(maps, {}, &CreativeModeFile::lastModified);

	return maps;
}

std::string CreativeModePlugin::GetKismetStringValue(const std::string& name)
{
	auto kismetVar = mapVariables.find(name);
	if (kismetVar == mapVariables.end() or !kismetVar->second.IsString()) {
		return "";
	}
	return kismetVar->second.GetString();
}

void CreativeModePlugin::SetLevelListInfoVariable()
{
	std::stringstream ss;
	auto maps = GetAllMaps();
	for (const auto& [index, map] : std::views::enumerate(maps)) {
		ss << map.GetMapString();
		if (index < maps.size() - 1)
			ss << ",";
	}

	LOG("Level list str {}", ss.str());
	auto listVar = mapVariables.find("LevelsListInfo");
	if (listVar != mapVariables.end() and listVar->second.IsString())
		listVar->second.SetString(ss.str());
}

void CreativeModePlugin::PlaceObject()
{
	auto sequence = gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) return;
	if (objectsToLoad.empty()) {
		LOG("No more objects to load");
		sequence.ActivateRemoteEvents("PluginControl_ListsPopulated");
		return;
	}

	const auto& [objName, objPos] = objectsToLoad.front();
	LOG("objname {}, objpos {}", objName, objPos.GetDebugString());
	std::string availListName = availListPrefix + objName;

	//TODO place object
	auto availList = mapVariables.find(availListName);
	if (availList == mapVariables.end() or !availList->second.IsObjectList() or availList->second.GetObjectList().Count() == 0) {
		LOG("{} is not a valid object list.", availListName);
		return;
	}

	LOG("{} is a valid object list", availListName);

	auto firstItem = availList->second.GetObjectList().Get(0);
	if (!firstItem.IsActor()) {
		LOG("First item found in {} is not an actor.", availListName);
		return;
	}

	LOG("first item is an actor");

	firstItem.GetActor().SetLocation(objPos.location);
	firstItem.GetActor().SetRotation(objPos.rotation);

	auto addedVar = mapVariables.find("PopulationAdded");
	if (addedVar != mapVariables.end() and addedVar->second.IsInt())
		addedVar->second.SetInt(addedVar->second.GetInt() + 1);

	sequence.ActivateRemoteEvents(objName);
	objectsToLoad.pop();
}

void CreativeModePlugin::LoadMap()
{
	auto mapName = GetKismetStringValue("MapNameToLoad");
	if (mapName == "") {
		LOG("No map found to load");
		//No map given
		return;
	}

	while (!objectsToLoad.empty())
		objectsToLoad.pop();

	if (!IsMapFolderValid()) {
		LOG("The maps folder '{}' either doesn't exist or isn't a folder", pluginSettings.mapsFolder);
		return;
	}

	std::ifstream dataFile(std::filesystem::path(pluginSettings.mapsFolder) / (mapName + fileExtension));
	if (dataFile.is_open()) {
		std::string line;
		//author
		std::getline(dataFile, line);
		//version
		std::getline(dataFile, line);
		std::string curObjList = "";
		while (std::getline(dataFile, line)) {
			if (line.find(' ') != std::string::npos) {
				auto curObj = Position(line);
				LOG("Adding obj to q {} {}", curObjList, curObj.GetDebugString());
				objectsToLoad.push({ curObjList, curObj });
				continue;
			}
			curObjList = line;
		}
	}
	dataFile.close();

	auto sizeVar = mapVariables.find("PopulationSize");
	if (sizeVar != mapVariables.end() and sizeVar->second.IsInt())
		sizeVar->second.SetInt(objectsToLoad.size());

	auto addedVar = mapVariables.find("PopulationAdded");
	if (addedVar != mapVariables.end() and addedVar->second.IsInt())
		addedVar->second.SetInt(0);
	
	PlaceObject();
}

void CreativeModePlugin::SaveMap()
{
	auto author = GetKismetStringValue("MapCreatorInfo");
	auto mapName = GetKismetStringValue("MapNameToSave");
	std::erase_if(mapName, [](char c) {
		return c == '\\' or c == '/' or c == ':' or c == '*' or c == '?' or c == '"' or c == '<' or c == '>' or c == '|' or c == '.';
		});

	if (author == "" or mapName == "") {
		LOG("No author {} or mapname {} given", author, mapName);
		return;
	}

	if (!IsMapFolderValid()) {
		LOG("The maps folder '{}' either doesn't exist or isn't a folder", pluginSettings.mapsFolder);
		return;
	}

	std::ofstream mapFile(std::filesystem::path(pluginSettings.mapsFolder) / (mapName + fileExtension));
	if (mapFile.is_open()) {
		mapFile << author << "\n";
		mapFile << "1" << "\n";
		for (auto& [varName, varValue] : mapVariables) {
			if (!varValue.IsObjectList() or varName.rfind(objectListPrefix, 0) != 0 or varName == unusedList) continue;

			auto curObjList = varValue.GetObjectList();
			std::string parsedName = varName.substr(objectListPrefix.length(), std::string::npos);

			if (parsedName == "ALL") {
				LOG("Not saving the all obj list");
				continue;
			}

			if (curObjList.Count() == 0) {
				LOG("Object list '{}' has no objects. Not saving list.", varName);
				continue;
			}

			mapFile << parsedName << "\n";
			LOG("Saving object list {} with size {}", parsedName, std::to_string(curObjList.Count()));

			for (int i = 0; i < curObjList.Count(); ++i) {
				auto curObj = curObjList.Get(i);
				if (curObj.IsNull()) {
					LOG("Object is found to be null :(");
					continue;
				}
				if (!curObj.IsActor()) {
					LOG("Not an object :(");
					continue;
				}

				auto obj = curObj.GetActor();
				auto objPos = Position(obj.GetLocation(), obj.GetRotation());
				mapFile << objPos.GetPositionString() << "\n";
				LOG("Saving object at index {} with {}", i, objPos.GetDebugString());
			}
		}
	}
	mapFile.close();

	auto sequence = gameWrapper->GetMainSequence();
	if (sequence.memory_address == NULL) { return; }
	sequence.ActivateRemoteEvents("PluginControl_Saved");
}

bool CreativeModePlugin::IsMapFolderValid() const
{
	return std::filesystem::exists(pluginSettings.mapsFolder) and std::filesystem::is_directory(pluginSettings.mapsFolder);
}

bool CreativeModePlugin::LoadSettings()
{
	if (!std::filesystem::exists(settingsFile)) {
		return false;
	}

	auto dataIn = std::ifstream(settingsFile);
	nlohmann::json j;
	if (dataIn.is_open()) {
		dataIn >> j;
	}
	dataIn.close();
	pluginSettings = j.get<Settings>();

	return pluginSettings.IsCurrentVersion();
}

void CreativeModePlugin::SaveSettings() const
{
	nlohmann::json j = pluginSettings;
	auto out = std::ofstream(settingsFile);
	if (out.is_open()) {
		out << j.dump();
	}
	out.close();
}

void CreativeModePlugin::RenderSettings()
{
	static bool isFolderValid;
	ImGui::TextUnformatted("Creative Mode Plugin Settings");
	if (ImGui::InputText("Maps Folder:", &pluginSettings.mapsFolder)) {
		//Maybe too much for typing
		SaveSettings();
	}
	ImGui::SameLine();
	if (ImGui::Button("Verify Folder")) {
		isFolderValid = IsMapFolderValid();
		ImGui::OpenPopup("Folder Status");
	}
	if (ImGui::Checkbox("Plugin Logging Allowed", &pluginSettings.loggingAllowed)) {
		SaveSettings();
		loggingIsAllowed = pluginSettings.loggingAllowed;
	}

	if (ImGui::BeginPopupModal("Folder Status", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (isFolderValid) {
			ImGui::TextUnformatted("Folder Found!");
		}
		else
			ImGui::TextUnformatted("Folder Not Found");

		if (ImGui::Button("Close##TasPluginpopup4", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}
