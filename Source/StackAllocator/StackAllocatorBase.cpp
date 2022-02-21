#include "StackAllocatorBase.hpp"

#include "AllocatorData.hpp"
#include "MemoryManager.hpp"

namespace Memory
{

StackAllocatorBase::StackAllocatorBase(Size totalSize, const std::shared_ptr<MemoryManager> memoryManager, const Size defaultAlignment,
                                       const char* debugName)
    : m_MemoryManager(memoryManager), m_Data(std::make_shared<AllocatorData>(debugName, totalSize)), m_DefaultAlignment(defaultAlignment)
{
    m_StartPtr     = malloc(m_Data->totalSize);
    m_StartAddress = reinterpret_cast<UIntPtr>(m_StartPtr);
    m_EndAddress   = m_StartAddress + totalSize;

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

void StackAllocatorBase::SetDeafaultAlignment(Size alignment) { m_DefaultAlignment = alignment; }

Size StackAllocatorBase::GetUsedSize() const { return m_Data->usedSize; }

Size StackAllocatorBase::GetTotalSize() const { return m_Data->totalSize; }

std::string StackAllocatorBase::GetDebugName() const { return m_Data->debugName; }

void StackAllocatorBase::SetCurrentOffset(Size offset)
{
    m_CurrentOffset = offset;

    m_Data->usedSize  = m_CurrentOffset;
    m_Data->peakUsage = std::max(m_Data->peakUsage, m_Data->usedSize);
}
} // namespace Memory
bool Memory::StackAllocatorBase::OwnsAddress(UIntPtr address) { return address >= m_StartAddress && address <= m_EndAddress; }
