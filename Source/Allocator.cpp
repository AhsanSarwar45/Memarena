#include "PCH.hpp"

#include "Allocator.hpp"

#include "Source/MemoryManager.hpp"

#include "Source/Assert.hpp"

namespace Memarena::Internal
{
Allocator::Allocator(Size totalSize, const std::string& debugName, bool isMemoryTrackingEnabled)
    : m_IsMemoryTrackingEnabled(isMemoryTrackingEnabled)
{
    MEMARENA_ASSERT(totalSize <= std::numeric_limits<Offset>::max(),
                    "Error: Max size of allocator cannot be more than %d! Value passed was %d.\n", std::numeric_limits<Offset>::max(),
                    totalSize);

    MEMARENA_ASSERT(totalSize > 0, "Error: Max size of allocator must be more than 0! Value passed was %d", totalSize);

    m_Data     = std::make_shared<AllocatorData>(debugName, totalSize);
    m_StartPtr = malloc(m_Data->totalSize);

    if (isMemoryTrackingEnabled)
    {
        s_MemoryManager.RegisterAllocator(m_Data);
    }
}

Allocator::~Allocator()
{
    if (m_IsMemoryTrackingEnabled)
    {
        s_MemoryManager.UnRegisterAllocator(m_Data);
    }
    free(m_StartPtr);
}

void Allocator::SetUsedSize(Size size)
{
    m_Data->usedSize  = size;
    m_Data->peakUsage = std::max(m_Data->peakUsage, m_Data->usedSize);

    if (m_IsMemoryTrackingEnabled)
    {
        s_MemoryManager.InvalidateUsedSizeCache();
    }
}

} // namespace Memarena::Internal
