#include "Benchmark.h"
#include "LinearAllocator.h"
#include "Log.h"
#include "PoolAllocator.h"
#include <chrono>
#include <new>
#include <vector>

namespace
{
	struct Particle
	{
		float pos[3];
		float vel[3];
		float life;
	};

	template <typename F>
	double TimeMs(F&& f)
	{
		const auto start = std::chrono::steady_clock::now();
		f();
		const auto end = std::chrono::steady_clock::now();
		return std::chrono::duration<double, std::milli>(end - start).count();
	}
}

void RunAllocatorBenchmark()
{
	constexpr std::size_t count = 100'100;
	constexpr int rounds = 50;

	std::vector<Particle*> ptrs(count);
	std::size_t sink = 0; // stops the compiler from optimizing the work away

	const double newMs = TimeMs([&]
	{
		for (int r = 0; r < rounds; ++r)
		{
			for (std::size_t i = 0; i < count; ++i)
			{
				ptrs[i] = new Particle{};
				ptrs[i]->life = static_cast<float>(i);
			}
			for (std::size_t i = 0; i < count; ++i)
			{
				sink += static_cast<std::size_t>(ptrs[i]->life);
				delete ptrs[i];
			}
		}
	});

	PoolAllocator<Particle> pool(count);
	const double poolMs = TimeMs([&]
	{
		for (int r = 0; r < rounds; ++r)
		{
			for (std::size_t i = 0; i < count; ++i)
			{
				ptrs[i] = pool.Create();
				ptrs[i]->life = static_cast<float>(i);
			}
			for (std::size_t i = 0; i < count; ++i)
			{
				sink += static_cast<std::size_t>(ptrs[i]->life);
				pool.Destroy(ptrs[i]);
			}
		}
	});

	LinearAllocator linear(count * sizeof(Particle));
	const double linearMs = TimeMs([&]
	{
		for (int r = 0; r < rounds; ++r)
		{
			for (std::size_t i = 0; i < count; ++i)
			{
				void* mem = linear.Allocate(sizeof(Particle), alignof(Particle));
				ptrs[i] = new (mem) Particle{};
				ptrs[i]->life = static_cast<float>(i);
			}
			for (std::size_t i = 0; i < count; ++i)
				sink += static_cast<std::size_t>(ptrs[i]->life);
			linear.Reset();
		}
	});

	LOG_INFO("new/delete	: {:.2f} ms", newMs);
	LOG_INFO("pool			: {:.2f} ms ({:.1f}x faster)", poolMs, newMs / poolMs);
	LOG_INFO("linear		: {:.2f} ms ({:.1f}x faster)", linearMs, newMs / linearMs);
	LOG_INFO("(checksum {})", sink);
}