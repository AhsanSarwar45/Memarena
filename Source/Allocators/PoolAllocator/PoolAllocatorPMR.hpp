#pragma once

#include <memory_resource>

#include "PoolAllocator.hpp"

namespace Memarena
{
template <PoolAllocatorPolicy policy = PoolAllocatorPolicy::Default>
class PoolAllocatorPMR : public std::pmr::memory_resource
{
  public:
    explicit PoolAllocatorPMR(const Size totalSize, const std::string& debugName = "PoolAllocatorPMR")
        : m_PoolAllocator(totalSize, debugName)
    {
    }
    void*              do_allocate(size_t bytes, size_t alignment) override { return m_PoolAllocator.Allocate(bytes, alignment); }
    void               do_deallocate(void* ptr, size_t /*bytes*/, size_t /*alignment*/) override { m_PoolAllocator.Deallocate(ptr); }
    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }

  private:
    PoolAllocator<policy> m_PoolAllocator;
};

} // namespace Memarena