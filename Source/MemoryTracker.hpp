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

    static void InvalidateTotalAllocatedSizeCache();

    [[nodiscard]] static Size                   GetTotalAllocatedSize();
    [[nodiscard]] static const AllocatorVector& GetAllocators();
    [[nodiscard]] static const AllocatorVector& GetBaseAllocators();

    static void Reset();
    static void ResetAllocators();
    static void ResetBaseAllocators();

  private:
    static std::mutex      m_Mutex;
    static AllocatorVector m_Allocators;
    static AllocatorVector m_BaseAllocators;
    static Cache<Size>     m_TotalAllocatedSize;
};
} // namespace Memarena