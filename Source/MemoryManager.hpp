#pragma once

#include <memory>
#include <vector>

#include "Aliases.hpp"

namespace Memarena
{
struct AllocatorData;

using AllocatorVector = std::vector<std::shared_ptr<AllocatorData>>;

class MemoryManager
{
  public:
    explicit MemoryManager(Size applicationBudget = 0);

    void RegisterAllocator(const std::shared_ptr<AllocatorData>& allocatorData);
    void UnRegisterAllocator(const std::shared_ptr<AllocatorData>& allocatorData);

    inline void IncreaseTotalSize(const Size size) { m_TotalAllocatedSize += size; }

    [[nodiscard]] Size                          GetUsedAllocatedSize() const;
    [[nodiscard]] inline Size                   GetTotalAllocatedSize() const { return m_TotalAllocatedSize; }
    [[nodiscard]] inline Size                   GetApplicationMemoryBudget() const { return m_ApplicationBudget; }
    [[nodiscard]] inline const AllocatorVector& GetAllocators() const { return m_Allocators; }

  private:
    AllocatorVector m_Allocators;

    Size m_ApplicationBudget; // if 0, means no budget limit
    Size m_TotalAllocatedSize;
};
} // namespace Memarena