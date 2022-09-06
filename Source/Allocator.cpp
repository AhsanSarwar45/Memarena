#include "Allocators/Mallocator/Mallocator.hpp"
#include "PCH.hpp"

#include "Allocator.hpp"

#include "Policies/Policies.hpp"
#include "Source/MemoryTracker.hpp"

#include "Source/Assert.hpp"
#include <memory>

namespace Memarena
{
Allocator::Allocator(Size totalSize, const std::string& debugName, bool isBaseAllocator)
{
    MEMARENA_ASSERT(totalSize <= std::numeric_limits<Offset>::max(),
                    "Error: Max size of allocator cannot be more than %u! Value passed was %u.\n", std::numeric_limits<Offset>::max(),
                    totalSize);

    MEMARENA_ASSERT(totalSize >= 0, "Error: Max size of allocator must be >= 0! Value passed was %d", totalSize);

    m_Data =
        std::make_shared<AllocatorData>(AllocatorData{.debugName = debugName, .totalSize = totalSize, .isBaseAllocator = isBaseAllocator});

    MemoryTracker::RegisterAllocator(m_Data);
}

Allocator::~Allocator() { MemoryTracker::UnRegisterAllocator(m_Data); }

void Allocator::SetUsedSize(Size size)
{
    m_Data->usedSize  = size;
    m_Data->peakUsage = std::max(m_Data->peakUsage, m_Data->usedSize);
}

void Allocator::SetTotalSize(Size size)
{
    m_Data->totalSize = size;
    MemoryTracker::InvalidateTotalAllocatedSizeCache();
}

void Allocator::AddAllocation(const Size size, const std::string& category, const SourceLocation& sourceLocation)
{
    m_Data->allocations.push_back({sourceLocation, category, size});
    m_Data->allocationCount++;
}

const std::shared_ptr<Allocator> Allocator::m_DefaultAllocator =
    std::make_shared<Mallocator<MallocatorPolicy::Default>>("DefaultMallocator");

} // namespace Memarena
