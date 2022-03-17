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

class MemoryTracker
{
  public:
    MemoryTracker() = default;

    static void RegisterAllocator(const std::shared_ptr<AllocatorData>& allocatorData);
    static void UnRegisterAllocator(const std::shared_ptr<AllocatorData>& allocatorData);

    static inline void InvalidateUsedSizeCache() { m_TotalUsedSizeCache.invalidated = true; }

    [[nodiscard]] static Size                          GetTotalUsedSize();
    [[nodiscard]] static inline Size                   GetTotalAllocatedSize() { return m_TotalAllocatedSize; }
    [[nodiscard]] static inline const AllocatorVector& GetAllocators() { return m_Allocators; }

    static void Reset()
    {
        std::lock_guard<std::mutex> guard(m_Mutex);

        m_TotalAllocatedSize = 0;
        m_TotalUsedSizeCache = {0, false};
        m_Allocators.clear();
        m_Allocators.shrink_to_fit();
    }

  private:
    static inline std::mutex      m_Mutex;
    static inline AllocatorVector m_Allocators;
    static inline Cache<Size>     m_TotalUsedSizeCache = {0, false};
    static inline Size            m_TotalAllocatedSize = 0;
};
} // namespace Memarena