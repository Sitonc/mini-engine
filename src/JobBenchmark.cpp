#include "JobBenchmark.h"
#include "JobSystem.h"
#include "Log.h"
#include <chrono>
#include <cmath>
#include <vector>

namespace
{
    struct Vec3 { float x = 0, y = 0, z = 0; };

    constexpr float dt = 1.0f / 60.0f;
    using Kernel = void (*)(Vec3*, const Vec3*, std::size_t, std::size_t);

    // Memory-bound: almost no math per byte loaded.
    void CheapKernel(Vec3* p, const Vec3* v, std::size_t b, std::size_t e)
    {
        for (std::size_t i = b; i < e; ++i)
        {
            p[i].x += v[i].x * dt;
            p[i].y += v[i].y * dt;
            p[i].z += v[i].z * dt;
        }
    }

    // Compute-bound: lots of math per element.
    void HeavyKernel(Vec3* p, const Vec3* v, std::size_t b, std::size_t e)
    {
        for (std::size_t i = b; i < e; ++i)
        {
            p[i].x += std::sin(p[i].y + v[i].x) * dt;
            p[i].y += std::cos(p[i].x + v[i].y) * dt;
            p[i].z += std::sqrt(p[i].x * p[i].x + p[i].y * p[i].y + 1.0f) * dt * 0.001f;
        }
    }

    template <typename F>
    double TimeMs(F&& f)
    {
        const auto start = std::chrono::steady_clock::now();
        f();
        return std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
    }

    // Parallel result must equal the serial result (within float tolerance).
    bool Matches(JobSystem& jobs, Kernel k, std::size_t n)
    {
        std::vector<Vec3> a(n), b(n), v(n, Vec3{ 1, 2, 3 });
        k(a.data(), v.data(), 0, n);
        jobs.ParallelFor(n, 4096, [&](std::size_t bg, std::size_t en)
            {
                k(b.data(), v.data(), bg, en);
            });
        for (std::size_t i = 0; i < n; ++i)
            if (std::fabs(a[i].x - b[i].x) > 1e-4f ||
                std::fabs(a[i].y - b[i].y) > 1e-4f ||
                std::fabs(a[i].z - b[i].z) > 1e-4f)
                return false;
        return true;
    }

    void Compare(const char* name, JobSystem& jobs, Kernel k, std::size_t n, int rounds)
    {
        std::vector<Vec3> pos(n), vel(n, Vec3{ 1, 2, 3 });
        Vec3* p = pos.data();
        const Vec3* v = vel.data();

        LOG_INFO("{}: results match serial: {}", name, Matches(jobs, k, 100'000) ? "yes" : "NO");

        const double serial = TimeMs([&] { for (int r = 0; r < rounds; ++r) k(p, v, 0, n); });
        LOG_INFO("{}: serial          : {:.2f} ms", name, serial);

        for (const std::size_t grain : {std::size_t{ 1024 }, std::size_t{ 16384 }, std::size_t{ 262144 }})
        {
            const double par = TimeMs([&]
                {
                    for (int r = 0; r < rounds; ++r)
                        jobs.ParallelFor(n, grain, [&](std::size_t b, std::size_t e) { k(p, v, b, e); });
                });
            LOG_INFO("{}: parallel g={:<6}: {:.2f} ms ({:.2f}x)", name, grain, par, serial / par);
        }
        LOG_INFO("{}: (checksum {})", name, p[n / 2].x + p[n / 2].y + p[n / 2].z);
    }
}

void RunJobBenchmark()
{
    JobSystem jobs;
    LOG_INFO("workers: {} (+ calling thread)", jobs.WorkerCount());
    constexpr std::size_t n = 1'000'000;
    Compare("cheap", jobs, CheapKernel, n, 50);
    Compare("heavy", jobs, HeavyKernel, n, 20);
}