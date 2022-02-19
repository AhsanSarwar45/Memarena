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
    const UIntPtr currentAddress = reinterpret_cast<UIntPtr>(m_HeadPtr) + m_Offset;

    // The padding includes alignment as well as the header
    const UInt8 padding = CalculatePaddingWithHeader(currentAddress, alignment, sizeof(AllocationHeader));

    const Size totalSizeAfterAllocation = m_Offset + padding + size;
    if (totalSizeAfterAllocation > m_Data->totalSize) // Check if this allocation will overflow the stack allocator
    {
        return nullptr; // Not enough space in the stack allocator, so return nullptr
    }

    const UIntPtr nextAddress   = currentAddress + padding;
    const UIntPtr headerAddress = nextAddress - sizeof(AllocationHeader);
    // Construct the header at 'headerAdress' using placement new operator
    const AllocationHeader* headerPtr = new (reinterpret_cast<void*>(headerAddress)) AllocationHeader(padding);

    m_Offset = totalSizeAfterAllocation;

    m_Data->usedSize  = m_Offset;
    m_Data->peakUsage = std::max(m_Data->peakUsage, m_Data->usedSize);

    return reinterpret_cast<void*>(nextAddress);
}

void StackAllocator::Deallocate(void* ptr)
{

    const UIntPtr           currentAddress   = reinterpret_cast<UIntPtr>(ptr);
    const UIntPtr           headerAddress    = currentAddress - sizeof(AllocationHeader);
    const AllocationHeader* allocationHeader = reinterpret_cast<AllocationHeader*>(headerAddress);

    const Size currentOffset = currentAddress - reinterpret_cast<UIntPtr>(m_HeadPtr);

    // Move offset back by removing the padding and the header
    m_Offset         = currentOffset - allocationHeader->padding;
    m_Data->usedSize = m_Offset;
}

Size StackAllocator::GetUsedSize() const { return m_Data->usedSize; }
Size StackAllocator::GetTotalSize() const { return m_Data->totalSize; }
} // namespace Memory