#include "StackAllocatorSafe.hpp"

#include "AllocatorData.hpp"
#include "Utility/Alignment.hpp"

#include "Assert.hpp"

namespace Memory
{

StackAllocatorSafe::StackAllocatorSafe(const Size totalSize, const std::shared_ptr<MemoryManager> memoryManager, const char* debugName)
    : StackAllocatorBase(totalSize, memoryManager, debugName)
{
}

StackPtr<void> StackAllocatorSafe::Allocate(const Size size, const Alignment alignment)
{
    const UInt32  m_InitialOffset = m_CurrentOffset;
    const UIntPtr baseAddress     = m_StartAddress + m_CurrentOffset;

    const UIntPtr alignedAddress = CalculateAlignedAddress(baseAddress, alignment);
    const Padding padding        = alignedAddress - baseAddress;

    const Size totalSizeAfterAllocation = m_CurrentOffset + padding + size;

    // Check if this allocation will overflow the stack allocator
    MEMORY_MANAGER_ASSERT(totalSizeAfterAllocation <= m_Data->totalSize, "Error: The allocator %s is out of memory!\n",
                          m_Data->debugName.c_str());

    SetCurrentOffset(totalSizeAfterAllocation);

    void* allocatedPtr = reinterpret_cast<void*>(alignedAddress);

    return {.ptr = allocatedPtr, .startOffset = m_InitialOffset, .endOffset = m_CurrentOffset};
}

void StackAllocatorSafe::Deallocate(StackPtr<void> ptrBlock)
{
    MEMORY_MANAGER_ASSERT(ptrBlock.ptr, "Error: Cannot deallocate nullptr!\n");

    // Check if this allocator owns the pointer
    MEMORY_MANAGER_ASSERT(OwnsAddress(reinterpret_cast<UIntPtr>(ptrBlock.ptr)), "Error: The allocator %s does not own the pointer %d!\n",
                          m_Data->debugName.c_str(), reinterpret_cast<UIntPtr>(ptrBlock.ptr));

    MEMORY_MANAGER_ASSERT(ptrBlock.endOffset == m_CurrentOffset, "Error: Attempt to deallocate in wrong order in the stack allocator %s!\n",
                          m_Data->debugName.c_str());

    SetCurrentOffset(ptrBlock.startOffset);
}

} // namespace Memory