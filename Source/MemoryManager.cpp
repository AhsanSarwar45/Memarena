#include "PCH.hpp"

#include "MemoryManager.hpp"

#include "AllocatorData.hpp"

namespace Memarena
{

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

Size MemoryManager::GetTotalUsedSize() const
{
    if (m_TotalUsedSizeCache.invalidated)
    {
        Size usedSize = 0;
        for (const auto& it : m_Allocators)
        {
            usedSize += it->usedSize;
        }

        m_TotalUsedSizeCache.value = usedSize;
    }

    return m_TotalUsedSizeCache.value;
}
} // namespace Memarena