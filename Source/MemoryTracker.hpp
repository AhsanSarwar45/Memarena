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

    static inline void InvalidateTotalAllocatedSizeCache() { m_TotalAllocatedSize.invalidated = true; }

    [[nodiscard]] static Size                          GetTotalAllocatedSize();
    [[nodiscard]] static inline const AllocatorVector& GetAllocators() { return m_Allocators; }
    [[nodiscard]] static inline const AllocatorVector& GetBaseAllocators() { return m_BaseAllocators; }

    static void Reset()
    {
        std::lock_guard<std::mutex> guard(m_Mutex);

        m_Allocators.clear();
        m_Allocators.shrink_to_fit();
        m_BaseAllocators.clear();
        m_BaseAllocators.shrink_to_fit();
    }

    static void ResetAllocators()
    {
        std::lock_guard<std::mutex> guard(m_Mutex);

        m_Allocators.clear();
        m_Allocators.shrink_to_fit();
    }

    static void ResetBaseAllocators()
    {
        std::lock_guard<std::mutex> guard(m_Mutex);

        m_BaseAllocators.clear();
        m_BaseAllocators.shrink_to_fit();
    }

  private:
    static inline std::mutex      m_Mutex;
    static inline AllocatorVector m_Allocators;
    static inline AllocatorVector m_BaseAllocators;
    static inline Cache<Size>     m_TotalAllocatedSize = {0, false};
};
} // namespace Memarena