#include "Sim.h"
#include "Ecs.h"
#include "JobSystem.h"
#include "Log.h"
#include "Profiler.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

namespace
{
    struct Position { float x = 0, y = 0, z = 0; };
    struct Velocity { float x = 1, y = 2, z = 3; };

    constexpr float dt = 1.0f / 60.0f;

    void Move(Position& p, Velocity& v)
    {
        p.x += std::sin(p.y + v.x) * dt;
        p.y += std::cos(p.x + v.y) * dt;
        p.z += std::sqrt(p.x * p.x + p.y * p.y + 1.0f) * dt * 0.001f;
    }
}

void RunSim(bool parallel, double seconds)
{
    using Clock = std::chrono::steady_clock;
    using Ms = std::chrono::duration<double, std::milli>;

    PROFILER_THREAD_NAME("Main");
    JobSystem jobs;
    Registry reg;

    constexpr std::size_t n = 200'000;
    for (std::size_t i = 0; i < n; ++i)
    {
        const Entity e = reg.Create();
        reg.Add(e, Position{});
        reg.Add(e, Velocity{});
    }
    LOG_INFO("sim: {} entities, mode: {}, {} workers", n,
        parallel ? "parallel" : "serial", jobs.WorkerCount());

    constexpr double frameBudgetMs = 1000.0 / 60.0;
    const auto start = Clock::now();
    double totalWork = 0.0, worstWork = 0.0;
    std::size_t frames = 0;

    while (std::chrono::duration<double>(Clock::now() - start).count() < seconds)
    {
        const auto frameStart = Clock::now();
        {
            ZoneScopedN("Movement");
            if (parallel) reg.EachParallel<Position, Velocity>(jobs, 4096, Move);
            else          reg.Each<Position, Velocity>(Move);
        }
        const double work = Ms(Clock::now() - frameStart).count();
        totalWork += work;
        worstWork = std::max(worstWork, work);
        ++frames;

        FrameMark;
        // Pretend to be a 60 FPS game so the profiler shows work vs idle.
        if (work < frameBudgetMs)
            std::this_thread::sleep_for(Ms(frameBudgetMs - work));
    }

    LOG_INFO("sim: {} frames, average work {:.3f} ms, worst {:.3f} ms",
        frames, totalWork / static_cast<double>(frames), worstWork);
}