#include "Benchmark.h"
#include "EcsBenchmark.h"
#include "Window.h"
#include "Log.h"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <exception>

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

        using Clock = std::chrono::steady_clock;
        constexpr double fixedDt = 1.0 / 60.0;

        auto previous = Clock::now();
        double accumulator = 0.0;
        uint64_t updates = 0;

        while (window.PumpEvents())
        {
            auto now = Clock::now();
            accumulator += std::chrono::duration<double>(now - previous).count();
            previous = now;

            // Avoid the "spiral of death" after a long stall.
            if (accumulator > 0.25) accumulator = 0.25;

            while (accumulator >= fixedDt)
            {
                // Update(fixedDt) goes here later
                ++updates;
                accumulator -= fixedDt;
            }

            // Render() goes here later
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