#include "StackAllocatorSafe.hpp"

#include "AllocatorData.hpp"
#include "Utility/Padding.hpp"

#include "Assert.hpp"

namespace Memory
{

StackAllocatorSafe::StackAllocatorSafe(const Size totalSize, const std::shared_ptr<MemoryManager> memoryManager,
                                       const Size defaultAlignment, const char* debugName)
    : StackAllocatorBase(totalSize, memoryManager, defaultAlignment, debugName)
{
}

StackPtr<void> StackAllocatorSafe::Allocate(const Size size, const Size alignment)
{
    const UInt32  m_InitialOffset = m_CurrentOffset;
    const UIntPtr currentAddress  = m_StartAddress + m_CurrentOffset;

    // The padding includes alignment as well as the header
    const UInt8 padding = CalculatePadding(currentAddress, alignment);

    const Size totalSizeAfterAllocation = m_CurrentOffset + padding + size;
    if (totalSizeAfterAllocation > m_Data->totalSize) // Check if this allocation will overflow the stack allocator
    {
        return {.ptr = nullptr}; // Not enough space in the stack allocator, so return nullptr
    }

    const UIntPtr nextAddress = currentAddress + padding;

    SetCurrentOffset(totalSizeAfterAllocation);

    void* allocatedPtr = reinterpret_cast<void*>(nextAddress);

    return {.ptr = allocatedPtr, .startOffset = m_InitialOffset, .endOffset = m_CurrentOffset};
}

void StackAllocatorSafe::Deallocate(StackPtr<void> ptrBlock)
{
    if (ptrBlock.endOffset == m_CurrentOffset)
    {
        SetCurrentOffset(ptrBlock.startOffset);
    }
}

} // namespace Memory