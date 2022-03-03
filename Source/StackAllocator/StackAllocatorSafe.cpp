#include "StackAllocatorSafe.hpp"

#include "Source/AllocatorData.hpp"

#include "Source/Assert.hpp"

namespace Memarena
{

StackAllocatorSafe::StackAllocatorSafe(const Size totalSize, const std::shared_ptr<MemoryManager> memoryManager,
                                       const std::string& debugName)
    : StackAllocatorBase(totalSize, memoryManager, debugName)
{
}

// StackAllocatorSafe::StackAllocatorSafe(const Size totalSize, const std::string& debugName)
//     : StackAllocatorBase(totalSize, nullptr, debugName)
// {
// }

StackPtr<void> StackAllocatorSafe::Allocate(const Size size, const Alignment& alignment)
{
    const Offset  startOffset = m_CurrentOffset;
    const UIntPtr baseAddress = m_StartAddress + m_CurrentOffset;

    const UIntPtr alignedAddress = CalculateAlignedAddress(baseAddress, alignment);
    const Padding padding        = alignedAddress - baseAddress;

    const Size totalSizeAfterAllocation = m_CurrentOffset + padding + size;

    // Check if this allocation will overflow the stack allocator
    MEMARENA_ASSERT(totalSizeAfterAllocation <= m_Data->totalSize, "Error: The allocator %s is out of memory!\n",
                    m_Data->debugName.c_str());

    SetCurrentOffset(totalSizeAfterAllocation);

    void* allocatedPtr = reinterpret_cast<void*>(alignedAddress);
    return {.ptr = allocatedPtr, .startOffset = startOffset, .endOffset = m_CurrentOffset};
}

void StackAllocatorSafe::Deallocate(StackPtr<void> ptrBlock)
{
    MEMARENA_ASSERT(ptrBlock.ptr, "Error: Cannot deallocate nullptr!\n");

    // Check if this allocator owns the pointer
    MEMARENA_ASSERT(OwnsAddress(reinterpret_cast<UIntPtr>(ptrBlock.ptr)), "Error: The allocator %s does not own the pointer %d!\n",
                    m_Data->debugName.c_str(), reinterpret_cast<UIntPtr>(ptrBlock.ptr));

    MEMARENA_ASSERT(ptrBlock.endOffset == m_CurrentOffset, "Error: Attempt to deallocate in wrong order in the stack allocator %s!\n",
                    m_Data->debugName.c_str());

    SetCurrentOffset(ptrBlock.startOffset);
}

StackPtr<void> StackAllocatorSafe::AllocateArray(const Size objectCount, const Size objectSize, const Alignment& alignment)
{
    return Allocate(objectCount * objectSize, alignment);
}

void StackAllocatorSafe::DeallocateArray(StackPtr<void> ptr) { Deallocate(ptr); }

} // namespace Memarena