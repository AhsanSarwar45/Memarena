#include "PCH.hpp"

#include "MemoryManager.hpp"

#include "AllocatorData.hpp"

namespace Memarena
{
MemoryManager::MemoryManager(Size applicationBudget) : m_ApplicationBudget(applicationBudget), m_TotalAllocatedSize(0) {}

void MemoryManager::RegisterAllocator(const std::shared_ptr<AllocatorData>& allocatorData)
{
    m_TotalAllocatedSize += allocatorData->totalSize;
    m_Allocators.push_back(allocatorData);
}

void MemoryManager::UnRegisterAllocator(const std::shared_ptr<AllocatorData>& allocatorData)
{
    m_Allocators.erase(std::remove(m_Allocators.begin(), m_Allocators.end(), allocatorData), m_Allocators.end());

    m_TotalAllocatedSize -= allocatorData->totalSize;
}

Size MemoryManager::GetUsedAllocatedSize() const
{
    Size usedSize = 0;
    for (const auto& it : m_Allocators)
    {
        usedSize += it->usedSize;
    }

    return usedSize;
}
} // namespace Memarena