#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>

class LinearAllocator
{
public:
	explicit LinearAllocator(std::size_t capacity)
		: m_buffer(std::make_unique<std::byte[]>(capacity)), m_capacity(capacity) {}

	// alignement must be a power of two. Returns nullptr when out of space.
	void* Allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t))
	{
		assert((alignment & (alignment - 1)) == 0);
		const auto base = reinterpret_cast<std::uintptr_t>(m_buffer.get());
		const std::uintptr_t aligned = (base + m_offset + alignment - 1) & ~(alignment - 1);
		const std::size_t newOffset = (aligned - base) + size;
		if (newOffset > m_capacity) return nullptr;
		m_offset = newOffset;
		return reinterpret_cast<void*>(aligned);
	}

	void Reset() { m_offset = 0; }
	std::size_t Used() const { return m_offset; }
	std::size_t Capacity() const { return m_capacity; }

private:
	std::unique_ptr<std::byte[]> m_buffer;
	std::size_t m_capacity;
	std::size_t m_offset = 0;
};