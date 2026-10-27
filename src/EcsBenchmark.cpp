#include "EcsBenchmark.h"
#include "Ecs.h"
#include "Log.h"
#include <chrono>
#include <vector>

namespace
{
    struct Position { float x = 0, y = 0, z = 0; };
    struct Velocity { float x = 1, y = 2, z = 3; };

    // Mimics a typical "fat" game object: the data we update is
    // mixed in with lots of data this loop doesn't need.
    struct GameObject
    {
        Position pos;
        Velocity vel;
        char other[128];
    };

    template <typename F>
    double TimeMs(F&& f)
    {
        const auto start = std::chrono::steady_clock::now();
        f();
        return std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
    }
}

void RunEcsBenchmark()
{
    {   // quick correctness check
        Registry reg;
        Entity a = reg.Create();
        reg.Add(a, Position{});
        bool ok = reg.Has<Position>(a);
        reg.Destroy(a);
        Entity b = reg.Create(); // reuses the slot with a new generation
        ok = ok && !reg.IsAlive(a) && reg.IsAlive(b) && !reg.Has<Position>(b);
        LOG_INFO("sanity check: {}", ok ? "passed" : "FAILED");
    }

    constexpr std::size_t n = 1'000'000;
    constexpr int rounds = 20;
    constexpr float dt = 1.0f / 60.0f;
    float sink = 0.0f;

    std::vector<GameObject> objects(n);
    const double aosMs = TimeMs([&]
        {
            for (int r = 0; r < rounds; ++r)
                for (auto& o : objects)
                {
                    o.pos.x += o.vel.x * dt;
                    o.pos.y += o.vel.y * dt;
                    o.pos.z += o.vel.z * dt;
                }
        });
    for (const auto& o : objects) sink += o.pos.x;

    Registry reg;
    for (std::size_t i = 0; i < n; ++i)
    {
        Entity e = reg.Create();
        reg.Add(e, Position{});
        reg.Add(e, Velocity{});
    }
    const double ecsMs = TimeMs([&]
        {
            for (int r = 0; r < rounds; ++r)
                reg.Each<Position, Velocity>([&](Position& p, Velocity& v)
                    {
                        p.x += v.x * dt;
                        p.y += v.y * dt;
                        p.z += v.z * dt;
                    });
        });
    reg.Each<Position, Velocity>([&](Position& p, Velocity&) { sink += p.x; });

    LOG_INFO("fat objects (AoS) : {:.2f} ms", aosMs);
    LOG_INFO("ECS packed arrays : {:.2f} ms ({:.1f}x faster)", ecsMs, aosMs / ecsMs);
    LOG_INFO("(checksum {})", sink);
}