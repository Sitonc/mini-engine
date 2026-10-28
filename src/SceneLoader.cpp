#include "SceneLoader.h"
#include "Log.h"
#include <fstream>
#include <utility>

using nlohmann::json;

bool SceneLoader::Load(const std::string& path, Registry& registry) const
{
	std::ifstream file(path);
	if (!file)
	{
		LOG_ERROR("Cannot open scene file: {}", path);
		return false;
	}

	Registry fresh;
	std::size_t count = 0;

	try
	{
		const json doc = json::parse(file);
		for (const auto& ent : doc.at("entities"))
		{
			const Entity e = fresh.Create();
			for (const auto& item : ent.at("components").items())
			{
				const auto it = m_deserializers.find(item.key());
				if (it == m_deserializers.end())
				{
					LOG_ERROR("{}: unknown component '{}'", path, item.key());
					return false;
				}
				it->second(fresh, e, item.value());
			}
			++count;
		}
	}
	catch (const json::exception& ex)
	{
		LOG_ERROR("{}: {}", path, ex.what());
		return false;
	}

	registry = std::move(fresh);
	LOG_INFO("Loaded {} entities from {}", count, path);
	return true;
}