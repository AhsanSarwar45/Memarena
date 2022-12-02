#pragma once

#include <memory_resource>

#include "PoolAllocator.hpp"

namespace Memarena
{
template <PoolAllocatorSettings Settings = poolAllocatorDefaultSettings>
class PoolAllocatorPMR : public std::pmr::memory_resource
{
  public:
    explicit PoolAllocatorPMR(const Size objectSize, const Size objectsPerBlock, const std::string& debugName = "PoolAllocatorPMR")
        : m_PoolAllocator(objectSize, objectsPerBlock, debugName)
    {
    }
    void* do_allocate(Size size, Size /*alignment*/) override { return m_PoolAllocator.AllocateArrayInternal(GetMinimumObjectCount(size)); }
    void  do_deallocate(void* ptr, Size size, Size /*alignment*/) override
    {
        m_PoolAllocator.DeallocateArrayInternal(ptr, GetMinimumObjectCount(size));
    }
    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override { return this == &other; }

    const PoolAllocator<Settings>& GetInternalAllocator() const { return m_PoolAllocator; }

  private:
    [[nodiscard]] Size GetMinimumObjectCount(Size bytes) const
    {
        Size objectSize = m_PoolAllocator.GetObjectSize();
        return (bytes + objectSize - 1) / objectSize;
    }

    PoolAllocator<Settings> m_PoolAllocator;
};

} // namespace Memarena