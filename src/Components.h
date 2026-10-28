#pragma once
#include <nlohmann/json.hpp>
#include <string>

namespace comp
{
	struct Name { std::string value; };
	struct Position { float x = 0, y = 0, z = 0; };
	struct Velocity { float x = 0, y = 0, z = 0; };

	// Generates to_json/from_json; missing keys fall back to the defaults above.
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Name, value)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Position, x, y, z)
	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Velocity, x, y, z)
}