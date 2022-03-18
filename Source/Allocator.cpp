#include "PCH.hpp"

#include "Allocator.hpp"

#include "Source/MemoryTracker.hpp"

#include "Source/Assert.hpp"

namespace Memarena
{
Allocator::Allocator(Size totalSize, const std::string& debugName)
{
    MEMARENA_ASSERT(totalSize <= std::numeric_limits<Offset>::max(),
                    "Error: Max size of allocator cannot be more than %d! Value passed was %d.\n", std::numeric_limits<Offset>::max(),
                    totalSize);

    MEMARENA_ASSERT(totalSize > 0, "Error: Max size of allocator must be more than 0! Value passed was %d", totalSize);

    m_Data     = std::make_shared<AllocatorData>(AllocatorData{.debugName = debugName, .totalSize = totalSize});
    m_StartPtr = malloc(m_Data->totalSize);

    MemoryTracker::RegisterAllocator(m_Data);
}

Allocator::~Allocator()
{
    MemoryTracker::UnRegisterAllocator(m_Data);

    free(m_StartPtr);
}

void Allocator::SetUsedSize(Size size)
{
    m_Data->usedSize  = size;
    m_Data->peakUsage = std::max(m_Data->peakUsage, m_Data->usedSize);

    MemoryTracker::InvalidateUsedSizeCache();
}

void Allocator::AddAllocation(const Size size, const std::string& category, const SourceLocation& sourceLocation)
{
    m_Data->allocations.push_back({sourceLocation, category, size});
    m_Data->allocationCount++;
}

} // namespace Memarena
