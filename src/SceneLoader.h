#pragma once
#include "Ecs.h"
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

class SceneLoader
{
public:
	template <typename T>
	void RegisterComponent(const std::string& name)
	{
		m_deserializers[name] = [](Registry& reg, Entity e, const nlohmann::json& j)
		{
			reg.Add(e, j.get<T>());
		};
	}

	// Returns false and leaves 'registry' untouched if the file is invalid.
	bool Load(const std::string& path, Registry& registry) const;

private:
	using Deserializer = std::function<void(Registry&, Entity, const nlohmann::json&)>;
	std::unordered_map < std::string, Deserializer> m_deserializers;
};