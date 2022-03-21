#pragma once

#include <memory_resource>

#include "StackAllocator.hpp"

namespace Memarena
{
template <StackAllocatorPolicy policy = StackAllocatorPolicy::Default>
class StackAllocatorPMR : public std::pmr::memory_resource
{
  public:
    explicit StackAllocatorPMR(const Size totalSize, const std::string& debugName = "StackAllocatorPMR")
        : m_StackAllocator(totalSize, debugName)
    {
    }
    void*              do_allocate(size_t bytes, size_t alignment) override { return m_StackAllocator.Allocate(bytes, alignment); }
    void               do_deallocate(void* ptr, size_t bytes, size_t alignment) override { m_StackAllocator.Deallocate(ptr); }
    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }

  private:
    StackAllocator<policy> m_StackAllocator;
};

} // namespace Memarena