#pragma once

#include <memory_resource>

#include "PoolAllocatorPMR.hpp"

namespace Memarena
{
template <Allocatable Object, PoolAllocatorPolicy policy = defaultPoolAllocatorPolicy>
class PoolAllocatorTemplatedPMR : public std::pmr::memory_resource
{
  public:
    explicit PoolAllocatorTemplatedPMR(const Size objectsPerBlock, const std::string& debugName = "PoolAllocatorPMR")
        : m_PoolAllocator(sizeof(Object), objectsPerBlock, debugName)
    {
    }
    void* do_allocate(size_t bytes, size_t alignment) override { return m_PoolAllocator.do_allocate(bytes, alignment); }
    void  do_deallocate(void* ptr, size_t bytes, size_t alignment) override { m_PoolAllocator.do_deallocate(ptr, bytes, alignment); }
    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }

  private:
    PoolAllocatorPMR<policy> m_PoolAllocator;
};

} // namespace Memarena