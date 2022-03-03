#include "PCH.hpp"

#include "StackAllocatorBase.hpp"

#include "Source/AllocatorData.hpp"
#include "Source/MemoryManager.hpp"

#include "Source/Assert.hpp"

namespace Memarena
{

StackAllocatorBase::StackAllocatorBase(const Size totalSize, const std::shared_ptr<MemoryManager> memoryManager,
                                       const std::string& debugName)
{
    MEMARENA_ASSERT(totalSize <= std::numeric_limits<Offset>::max(),
                    "Error: Max size of allocator cannot be more than %d! Value passed was %d.\n", std::numeric_limits<Offset>::max(),
                    totalSize);

    MEMARENA_ASSERT(totalSize > 0, "Error: Max size of allocator must be more than 0! Value passed was %d", totalSize);

    m_Data = std::make_shared<AllocatorData>(debugName, totalSize);

    m_StartPtr     = malloc(m_Data->totalSize);
    m_StartAddress = reinterpret_cast<UIntPtr>(m_StartPtr);
    m_EndAddress   = m_StartAddress + totalSize;

    m_MemoryManager = memoryManager;

    if (m_MemoryManager)
    {
        // Allows the memory manager to keep track of total allocated memory
        m_MemoryManager->RegisterAllocator(m_Data);
    }

    SetCurrentOffset(0);
}

StackAllocatorBase::~StackAllocatorBase()
{
    if (m_MemoryManager)
    {
        m_MemoryManager->UnRegisterAllocator(m_Data);
    }

    free(m_StartPtr);
}

void StackAllocatorBase::Reset() { SetCurrentOffset(0); }

Size StackAllocatorBase::GetUsedSize() const { return m_Data->usedSize; }

Size StackAllocatorBase::GetTotalSize() const { return m_Data->totalSize; }

std::string StackAllocatorBase::GetDebugName() const { return m_Data->debugName; }

void StackAllocatorBase::SetCurrentOffset(Offset offset)
{
    m_CurrentOffset = offset;

    m_Data->usedSize  = m_CurrentOffset;
    m_Data->peakUsage = std::max(m_Data->peakUsage, m_Data->usedSize);
}
bool StackAllocatorBase::OwnsAddress(UIntPtr address) { return address >= m_StartAddress && address <= m_EndAddress; }
} // namespace Memarena
