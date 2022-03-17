#include "PCH.hpp"

#include "MemoryTracker.hpp"

#include "AllocatorData.hpp"
#include <mutex>

namespace Memarena
{

void MemoryTracker::RegisterAllocator(const std::shared_ptr<AllocatorData>& allocatorData)
{
    std::lock_guard<std::mutex> guard(m_Mutex);

    m_TotalAllocatedSize += allocatorData->totalSize;
    m_Allocators.push_back(allocatorData);
}

void MemoryTracker::UnRegisterAllocator(const std::shared_ptr<AllocatorData>& allocatorData)
{
    std::lock_guard<std::mutex> guard(m_Mutex);

    m_Allocators.erase(std::remove(m_Allocators.begin(), m_Allocators.end(), allocatorData), m_Allocators.end());
    m_TotalAllocatedSize -= allocatorData->totalSize;
}

Size MemoryTracker::GetTotalUsedSize()
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