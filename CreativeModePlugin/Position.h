#pragma once
#include <bakkesmod/wrappers/wrapperstructs.h>

struct Position {
	Vector location;
	Rotator rotation;

	Position(const std::string& positionStr);
	Position(const Vector& loc, const Rotator& rot);
	std::string GetPositionString() const;
	std::string GetDebugString() const;
};