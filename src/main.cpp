#include "Benchmark.h"
#include "Components.h"
#include "Ecs.h"
#include "EcsBenchmark.h"
#include "FileWatcher.h"
#include "Log.h"
#include "SceneLoader.h"
#include "Window.h"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <exception>
#include <string>

int main(int argc, char** argv)
{
    if (argc > 1 && std::strcmp(argv[1], "--bench") == 0)
    {
        RunAllocatorBenchmark();
        return 0;
    }
    if (argc > 1 && std::strcmp(argv[1], "--ecs") == 0)
    {
        RunEcsBenchmark();
        return 0;
    }

    try
    {
        Window window("Mini Engine", 1280, 720);

        Registry registry;
        SceneLoader loader;
        loader.RegisterComponent<comp::Name>("Name");
        loader.RegisterComponent<comp::Position>("Position");
        loader.RegisterComponent<comp::Velocity>("Velocity");

        const std::string scenePath = std::string(ASSET_DIR) + "/scene.json";
        FileWatcher watcher(scenePath);
        loader.Load(scenePath, registry);

        using Clock = std::chrono::steady_clock;
        constexpr double fixedDt = 1.0 / 60.0;

        auto previous = Clock::now();
        double accumulator = 0.0;
        std::uint64_t updates = 0;

        while (window.PumpEvents())
        {
            const auto now = Clock::now();
            accumulator += std::chrono::duration<double>(now - previous).count();
            previous = now;
            if (accumulator > 0.25) accumulator = 0.25;

            while (accumulator >= fixedDt)
            {
                ++updates;

                if (updates % 30 == 0 && watcher.Changed())
                {
                    LOG_INFO("scene.json changed, reloading");
                    loader.Load(scenePath, registry);
                }

                registry.Each<comp::Position, comp::Velocity>(
                    [](comp::Position& p, comp::Velocity& v)
                    {
                        p.x += v.x * static_cast<float>(fixedDt);
                        p.y += v.y * static_cast<float>(fixedDt);
                        p.z += v.z * static_cast<float>(fixedDt);
                    });

                if (updates % 60 == 0)
                {
                    registry.Each<comp::Name, comp::Position>(
                        [](comp::Name& n, comp::Position& p)
                        {
                            LOG_INFO("{} at ({:.1f}, {:.1f}, {:.1f})", n.value, p.x, p.y, p.z);
                        });
                }

                accumulator -= fixedDt;
            }
        }

        LOG_INFO("Shutting down after {} updates", updates);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Fatal: {}", e.what());
        return 1;
    }
    return 0;
}