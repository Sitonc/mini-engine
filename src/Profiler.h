#pragma once
#include <tracy/Tracy.hpp>

// Names a thread in the profiler UI. Does nothing when profiling is off.
#ifdef TRACY_ENABLE
	#define PROFILER_THREAD_NAME(name) tracy::SetThreadName(name)
#else
	#define PROFILER_THREAD_NAME(name) ((void)0)
#endif