#include "pch.h"
#include "Position.h"
#include <sstream>
#include "Utility.h"

Position::Position(const std::string& positionStr)
{
	auto splitLocRot = Utility::SplitStringByChar(positionStr, ' ');
	auto locSplit = Utility::SplitStringByChar(splitLocRot[0], ',');
	auto rotSplit = Utility::SplitStringByChar(splitLocRot[1], ',');

	location.X = std::stof(locSplit[0]);
	location.Y = std::stof(locSplit[1]);
	location.Z = std::stof(locSplit[2]);

	rotation.Pitch = std::stoi(rotSplit[0]);
	rotation.Yaw = std::stoi(rotSplit[1]);
	rotation.Roll = std::stoi(rotSplit[2]);
}

Position::Position(const Vector& loc, const Rotator& rot): location(loc), rotation(rot)
{
}

std::string Position::GetPositionString() const
{
	std::stringstream ss;
	std::string locStr = std::vformat("{},{},{}", std::make_format_args(location.X, location.Y, location.Z));
	std::string rotStr = std::vformat("{},{},{}", std::make_format_args(rotation.Pitch, rotation.Yaw, rotation.Roll));
	ss << locStr << " " << rotStr;
	return ss.str();
}

std::string Position::GetDebugString() const
{
	std::stringstream ss;
	std::string locStr = std::vformat("Location: ({},{},{})", std::make_format_args(location.X, location.Y, location.Z));
	std::string rotStr = std::vformat("Rotation: ({},{},{})", std::make_format_args(rotation.Pitch, rotation.Yaw, rotation.Roll));
	ss << locStr << " " << rotStr;
	return ss.str();
}