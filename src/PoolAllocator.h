#pragma once
#include <cstddef>
#include <memory>
#include <new>
#include <utility>

template <typename T>
class PoolAllocator
{
	union Slot
	{
		Slot* next;
		alignas(T) std::byte storage[sizeof(T)];
	};

public:
	explicit PoolAllocator(std::size_t count)
		: m_slots(std::make_unique<Slot[]>(count))
	{
		for (std::size_t i = 0; i + 1 < count; ++i)
			m_slots[i].next = &m_slots[i + 1];
		m_slots[count - 1].next = nullptr;
		m_freeList = &m_slots[0];
	}

	template <typename... Args>
	T* Create(Args&&... args)
	{
		if (!m_freeList) return nullptr;
		Slot* slot = m_freeList;
		m_freeList = slot->next;
		return new (slot->storage) T(std::forward<Args>(args)...);
	}

	void Destroy(T* obj)
	{
		obj->~T();
		Slot* slot = reinterpret_cast<Slot*>(obj);
		slot->next = m_freeList;
		m_freeList = slot;
	}

private:
	std::unique_ptr<Slot[]> m_slots;
	Slot* m_freeList = nullptr;
};