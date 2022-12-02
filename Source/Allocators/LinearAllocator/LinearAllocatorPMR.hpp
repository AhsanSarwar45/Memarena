#pragma once

#include <memory_resource>

#include "LinearAllocator.hpp"

namespace Memarena
{
template <auto Settings = AllocatorSettings<LinearAllocatorPolicy>()>
class LinearAllocatorPMR : public std::pmr::memory_resource
{
  public:
    explicit LinearAllocatorPMR(const Size blockSize, const std::string& debugName = "LinearAllocatorPMR",
                                std::shared_ptr<Allocator> baseAllocator = Allocator::GetDefaultAllocator())
        : m_LinearAllocator(blockSize, debugName, baseAllocator)
    {
    }
    void*              do_allocate(size_t bytes, size_t alignment) override { return m_LinearAllocator.Allocate(bytes, alignment); }
    void               do_deallocate(void* ptr, size_t bytes, size_t alignment) override {}
    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }

    const LinearAllocator<Settings>& GetInternalAllocator() const { return m_LinearAllocator; }

  private:
    LinearAllocator<Settings> m_LinearAllocator;
};

} // namespace Memarena