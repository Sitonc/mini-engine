#pragma once
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class JobSystem
{
public:
    explicit JobSystem(unsigned workers = 0)
    {
        if (workers == 0)
        {
            const unsigned hc = std::thread::hardware_concurrency();
            workers = hc > 1 ? hc - 1 : 1; // the calling thread helps too
        }
        for (unsigned i = 0; i < workers; ++i)
            m_threads.emplace_back([this] { WorkerLoop(); });
    }

    ~JobSystem()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stop = true;
        }
        m_cv.notify_all();
        for (auto& t : m_threads) t.join();
    }

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;

    unsigned WorkerCount() const { return static_cast<unsigned>(m_threads.size()); }

    // fn(begin, end) is called once per chunk, possibly on different threads.
    // Chunks must not depend on each other. fn must not throw.
    template <typename F>
    void ParallelFor(std::size_t count, std::size_t grain, F&& fn)
    {
        if (count == 0) return;
        if (grain == 0) grain = 1;
        const std::size_t chunks = (count + grain - 1) / grain;
        std::atomic<std::size_t> remaining{ chunks };

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (std::size_t c = 0; c < chunks; ++c)
            {
                const std::size_t begin = c * grain;
                const std::size_t end = std::min(count, begin + grain);
                m_queue.push([&fn, &remaining, begin, end]
                    {
                        fn(begin, end);
                        remaining.fetch_sub(1, std::memory_order_acq_rel);
                    });
            }
        }
        m_cv.notify_all();

        // Help out instead of idling until everything is finished.
        while (remaining.load(std::memory_order_acquire) > 0)
        {
            if (!TryRunOne()) std::this_thread::yield();
        }
    }

private:
    bool TryRunOne()
    {
        std::function<void()> job;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_queue.empty()) return false;
            job = std::move(m_queue.front());
            m_queue.pop();
        }
        job();
        return true;
    }

    void WorkerLoop()
    {
        for (;;)
        {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this] { return m_stop || !m_queue.empty(); });
                if (m_stop && m_queue.empty()) return;
                job = std::move(m_queue.front());
                m_queue.pop();
            }
            job();
        }
    }

    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_stop = false;
};