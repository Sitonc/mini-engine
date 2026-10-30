#pragma once
#include "JobSystem.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Entity: index + generation. The generation makes stale handles detectable.
// ---------------------------------------------------------------------------
struct Entity
{
    std::uint32_t index = 0xFFFFFFFF;
    std::uint32_t generation = 0;
};

class EntityManager
{
public:
    Entity Create()
    {
        if (!m_free.empty())
        {
            const std::uint32_t i = m_free.back();
            m_free.pop_back();
            return { i, m_generations[i] };
        }
        m_generations.push_back(0);
        return { static_cast<std::uint32_t>(m_generations.size() - 1), 0 };
    }

    void Destroy(Entity e)
    {
        ++m_generations[e.index]; // invalidates every old handle to this slot
        m_free.push_back(e.index);
    }

    bool IsAlive(Entity e) const
    {
        return e.index < m_generations.size() && m_generations[e.index] == e.generation;
    }

private:
    std::vector<std::uint32_t> m_generations;
    std::vector<std::uint32_t> m_free;
};

// ---------------------------------------------------------------------------
// Component storage: one sparse set per component type.
// ---------------------------------------------------------------------------
class IPool
{
public:
    virtual ~IPool() = default;
    virtual void Remove(std::uint32_t entityIndex) = 0;
};

template <typename T>
class Pool : public IPool
{
public:
    static constexpr std::uint32_t kNone = 0xFFFFFFFF;

    T& Add(std::uint32_t e, T value)
    {
        if (e >= m_sparse.size()) m_sparse.resize(e + 1, kNone);
        if (m_sparse[e] != kNone)
        {
            m_data[m_sparse[e]] = std::move(value);
            return m_data[m_sparse[e]];
        }
        m_sparse[e] = static_cast<std::uint32_t>(m_data.size());
        m_entities.push_back(e);
        m_data.push_back(std::move(value));
        return m_data.back();
    }

    bool Has(std::uint32_t e) const { return e < m_sparse.size() && m_sparse[e] != kNone; }
    T& Get(std::uint32_t e) { return m_data[m_sparse[e]]; }

    void Remove(std::uint32_t e) override
    {
        if (!Has(e)) return;
        const std::uint32_t di = m_sparse[e];
        const std::uint32_t lastDense = static_cast<std::uint32_t>(m_data.size() - 1);
        if (di != lastDense) // swap-and-pop keeps the array packed
        {
            m_data[di] = std::move(m_data[lastDense]);
            m_entities[di] = m_entities[lastDense];
            m_sparse[m_entities[di]] = di;
        }
        m_data.pop_back();
        m_entities.pop_back();
        m_sparse[e] = kNone;
    }

    std::vector<T>& Data() { return m_data; }
    const std::vector<std::uint32_t>& Entities() const { return m_entities; }
    const std::vector<std::uint32_t>& Sparse() const { return m_sparse; }

private:
    std::vector<std::uint32_t> m_sparse;   // entity index -> dense index
    std::vector<std::uint32_t> m_entities; // dense index  -> entity index
    std::vector<T> m_data;                 // packed components
};

// ---------------------------------------------------------------------------
// Registry: creates entities, owns the pools, runs queries.
// ---------------------------------------------------------------------------
class Registry
{
public:
    Entity Create() { return m_entities.Create(); }
    bool IsAlive(Entity e) const { return m_entities.IsAlive(e); }

    void Destroy(Entity e)
    {
        if (!m_entities.IsAlive(e)) return;
        for (auto& p : m_pools)
            if (p) p->Remove(e.index);
        m_entities.Destroy(e);
    }

    template <typename T>
    T& Add(Entity e, T value) { return GetPool<T>().Add(e.index, std::move(value)); }

    template <typename T>
    bool Has(Entity e) { return m_entities.IsAlive(e) && GetPool<T>().Has(e.index); }

    template <typename T>
    T& Get(Entity e) { return GetPool<T>().Get(e.index); }

    // Runs f(A&, B&) for every entity that has both components.
    // Do not add or remove components of A or B inside the callback:
    // that would invalidate the arrays being iterated.
    template <typename A, typename B, typename F>
    void Each(F&& f)
    {
        auto& pa = GetPool<A>();
        auto& pb = GetPool<B>();
        A* dataA = pa.Data().data();
        const std::uint32_t* ents = pa.Entities().data();
        const std::size_t count = pa.Data().size();
        B* dataB = pb.Data().data();
        const std::uint32_t* sparseB = pb.Sparse().data();
        const std::size_t sparseCount = pb.Sparse().size();

        for (std::size_t i = 0; i < count; ++i)
        {
            const std::uint32_t e = ents[i];
            if (e >= sparseCount) continue;
            const std::uint32_t di = sparseB[e];
            if (di != Pool<B>::kNone) f(dataA[i], dataB[di]);
        }
    }

    // Same as Each, but split across the job system.
    // The callback may only modify the components it receives. It must not
    // add/remove components, create/destroy entities, or write shared state.
    template <typename A, typename B, typename F>
    void EachParallel(JobSystem& jobs, std::size_t grain, F&& f)
    {
        auto& pa = GetPool<A>();
        auto& pb = GetPool<B>();
        A* dataA = pa.Data().data();
        const std::uint32_t* ents = pa.Entities().data();
        const std::size_t count = pa.Data().size();
        B* dataB = pb.Data().data();
        const std::uint32_t* sparseB = pb.Sparse().data();
        const std::size_t sparseCount = pb.Sparse().size();

        jobs.ParallelFor(count, grain, [&](std::size_t begin, std::size_t end)
            {
                for (std::size_t i = begin; i < end; ++i)
                {
                    const std::uint32_t e = ents[i];
                    if (e >= sparseCount) continue;
                    const std::uint32_t di = sparseB[e];
                    if (di != Pool<B>::kNone) f(dataA[i], dataB[di]);
                }
            });
    }

private:
    template <typename T>
    static std::size_t TypeId()
    {
        static const std::size_t id = s_counter++;
        return id;
    }

    template <typename T>
    Pool<T>& GetPool()
    {
        const std::size_t id = TypeId<T>();
        if (id >= m_pools.size()) m_pools.resize(id + 1);
        if (!m_pools[id]) m_pools[id] = std::make_unique<Pool<T>>();
        return static_cast<Pool<T>&>(*m_pools[id]);
    }

    static inline std::size_t s_counter = 0;
    EntityManager m_entities;
    std::vector<std::unique_ptr<IPool>> m_pools;
};