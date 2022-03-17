#pragma once

#include <memory>
#include <vector>

#include "Aliases.hpp"

namespace Memarena
{
struct AllocatorData;

using AllocatorVector = std::vector<std::shared_ptr<AllocatorData>>;

template <typename T>
struct Cache
{
    T    value;
    bool invalidated = false;
};

class MemoryManager
{
  public:
    MemoryManager() = default;

    void RegisterAllocator(const std::shared_ptr<AllocatorData>& allocatorData);
    void UnRegisterAllocator(const std::shared_ptr<AllocatorData>& allocatorData);

    inline void InvalidateUsedSizeCache() { m_TotalUsedSizeCache.invalidated = true; }

    [[nodiscard]] Size                          GetTotalUsedSize() const;
    [[nodiscard]] inline Size                   GetTotalAllocatedSize() const { return m_TotalAllocatedSize; }
    [[nodiscard]] inline const AllocatorVector& GetAllocators() const { return m_Allocators; }

  private:
    AllocatorVector m_Allocators;

    Size                m_TotalAllocatedSize = 0;
    mutable Cache<Size> m_TotalUsedSizeCache = {0, false};
};
} // namespace Memarena