#include "StackAllocator.hpp"

#include "AllocatorData.hpp"
#include "MemoryManager.hpp"
#include "Utility/Padding.hpp"

namespace Memory
{
StackAllocator::StackAllocator(Size totalSize, std::shared_ptr<MemoryManager> memoryManager, const char* debugName)
    : m_MemoryManager(memoryManager), m_Data(std::make_shared<AllocatorData>(debugName, totalSize)), m_HeadPtr(malloc(m_Data->totalSize))
{

    if (m_MemoryManager)
    {
        // Allows the memory manager to keep track of total allocated memory
        m_MemoryManager->RegisterAllocator(m_Data);
    }

    m_Offset = 0;
}

StackAllocator::~StackAllocator()
{
    if (m_MemoryManager)
    {
        m_MemoryManager->UnRegisterAllocator(m_Data);
    }

    free(m_HeadPtr);
}

void* StackAllocator::Allocate(const Size size, const Size alignment)
{
    const Size currentAddress = (Size)m_HeadPtr + m_Offset;

    Size padding = CalculatePaddingWithHeader(currentAddress, alignment, sizeof(AllocationHeader));

    if (m_Offset + padding + size > m_Data->totalSize)
    {
        return nullptr;
    }
    m_Offset += padding;

    const Size nextAddress = currentAddress + padding;

    m_Offset += size;

    m_Data->usedSize = m_Offset;
    return reinterpret_cast<void*>(nextAddress);
}

void StackAllocator::Deallocate(const Size ptr)
{
    const Size initialOffset = m_Offset;

    // Move offset back to clear address
    const Size              headerAddress = ptr - sizeof(AllocationHeader);
    const AllocationHeader* allocationHeader{reinterpret_cast<AllocationHeader*>(headerAddress)};

    m_Offset         = ptr - allocationHeader->padding - (Size)m_HeadPtr;
    m_Data->usedSize = m_Offset;
}

Size StackAllocator::GetUsedSize() const { return m_Data->usedSize; }
Size StackAllocator::GetTotalSize() const { return m_Data->totalSize; }
} // namespace Memory