#pragma once

#include <memory_resource>

#include "LinearAllocator.hpp"

namespace Memarena
{
template <LinearAllocatorPolicy policy = LinearAllocatorPolicy::Default>
class LinearAllocatorPMR : public std::pmr::memory_resource
{
  public:
    explicit LinearAllocatorPMR(const Size totalSize, const std::string& debugName = "LinearAllocatorPMR")
        : m_LinearAllocator(totalSize, debugName)
    {
    }
    void*              do_allocate(size_t bytes, size_t alignment) override { return m_LinearAllocator.Allocate(bytes, alignment); }
    void               do_deallocate(void* ptr, size_t bytes, size_t alignment) override {}
    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }

  private:
    LinearAllocator<policy> m_LinearAllocator;
};

} // namespace Memarena