#include "PCH.hpp"

#include "MemoryTracker.hpp"

#include "AllocatorData.hpp"
#include <mutex>

namespace Memarena
{

void MemoryTracker::RegisterAllocator(const std::shared_ptr<AllocatorData>& allocatorData)
{
    std::lock_guard<std::mutex> guard(m_Mutex);

    if (allocatorData->isBaseAllocator)
    {
        m_BaseAllocators.push_back(allocatorData);
    }
    else
    {
        m_Allocators.push_back(allocatorData);
    }
}

void MemoryTracker::UnRegisterAllocator(const std::shared_ptr<AllocatorData>& allocatorData)
{
    std::lock_guard<std::mutex> guard(m_Mutex);

    if (allocatorData->isBaseAllocator)
    {
        m_BaseAllocators.erase(std::remove(m_BaseAllocators.begin(), m_BaseAllocators.end(), allocatorData), m_BaseAllocators.end());
    }
    else
    {
        m_Allocators.erase(std::remove(m_Allocators.begin(), m_Allocators.end(), allocatorData), m_Allocators.end());
    }
}

Size MemoryTracker::GetTotalAllocatedSize()
{
    if (m_TotalAllocatedSize.invalidated)
    {
        Size totalSize = 0;
        for (const auto& it : m_BaseAllocators)
        {
            totalSize += it->totalSize;
        }

        m_TotalAllocatedSize.value = totalSize;
    }

    return m_TotalAllocatedSize.value;
}

} // namespace Memarena