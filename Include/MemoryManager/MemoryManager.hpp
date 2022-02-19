#pragma once

#include "Aliases.hpp"

namespace Memory
{
struct AllocatorData;

using AllocatorVector = std::vector<std::shared_ptr<AllocatorData>>;

class MemoryManager
{
  public:
    explicit MemoryManager(Size applicationBudget = 0);
    ~MemoryManager();

    void RegisterAllocator(std::shared_ptr<AllocatorData> allocatorData);
    void UnRegisterAllocator(std::shared_ptr<AllocatorData> allocatorData);

    inline void IncreaseTotalSize(const Size size) { m_TotalAllocatedSize += size; }

    Size                          GetUsedAllocatedSize() const;
    inline Size                   GetTotalAllocatedSize() const { return m_TotalAllocatedSize; }
    inline Size                   GetApplicationMemoryBudget() const { return m_ApplicationBudget; }
    inline const AllocatorVector& GetAllocators() const { return m_Allocators; }

  private:
    AllocatorVector m_Allocators;

    Size m_ApplicationBudget; // if 0, means no budget limit
    Size m_TotalAllocatedSize;
};
} // namespace Memory