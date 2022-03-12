#include "PCH.hpp"

#include "Allocator.hpp"

#include "Source/AllocatorData.hpp"
#include "Source/MemoryManager.hpp"

#include "Source/Assert.hpp"

namespace Memarena::Internal
{
Allocator::Allocator(const Size totalSize, const std::shared_ptr<MemoryManager>& memoryManager, const std::string& debugName)
    : m_MemoryManager(memoryManager)
{
    MEMARENA_ASSERT(totalSize <= std::numeric_limits<Offset>::max(),
                    "Error: Max size of allocator cannot be more than %d! Value passed was %d.\n", std::numeric_limits<Offset>::max(),
                    totalSize);

    MEMARENA_ASSERT(totalSize > 0, "Error: Max size of allocator must be more than 0! Value passed was %d", totalSize);

    m_Data     = std::make_shared<AllocatorData>(debugName, totalSize);
    m_StartPtr = malloc(m_Data->totalSize);

    if (m_MemoryManager)
    {
        // Allows the memory manager to keep track of total allocated memory
        m_MemoryManager->RegisterAllocator(m_Data);
    }
}

Allocator::~Allocator()
{
    if (m_MemoryManager)
    {
        m_MemoryManager->UnRegisterAllocator(m_Data);
    }

    free(m_StartPtr);
}

Size Allocator::GetUsedSize() const { return m_Data->usedSize; }

Size Allocator::GetTotalSize() const { return m_Data->totalSize; }

std::string Allocator::GetDebugName() const { return m_Data->debugName; }

void Allocator::SetUsedSize(const Size size)
{
    m_Data->usedSize  = size;
    m_Data->peakUsage = std::max(m_Data->peakUsage, m_Data->usedSize);
}
} // namespace Memarena::Internal
