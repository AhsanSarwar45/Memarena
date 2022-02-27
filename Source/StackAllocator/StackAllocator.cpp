#include "StackAllocator.hpp"

#include "AllocatorData.hpp"

#include "Assert.hpp"

namespace Memarena
{

StackAllocator::StackAllocator(const Size totalSize, const std::shared_ptr<MemoryManager> memoryManager, const char* debugName)
    : StackAllocatorBase(totalSize, memoryManager, debugName)
{
}

void* StackAllocator::Allocate(const Size size, const Alignment& alignment)
{
    const UIntPtr baseAddress = m_StartAddress + m_CurrentOffset;

    // The padding includes alignment as well as the header
    const Padding padding = CalculateAlignedPaddingWithHeader(baseAddress, alignment, sizeof(Header));

    const Size totalSizeAfterAllocation = m_CurrentOffset + padding + size;
    // Check if this allocation will overflow the stack allocator
    MEMORY_MANAGER_ASSERT(totalSizeAfterAllocation <= m_Data->totalSize, "Error: The allocator %s is out of memory!\n",
                          m_Data->debugName.c_str());

    const UIntPtr alignedAddress = baseAddress + padding;
    AllocateHeader<Header>(alignedAddress, padding);

    SetCurrentOffset(totalSizeAfterAllocation);

    void* allocatedPtr = reinterpret_cast<void*>(alignedAddress);
    return allocatedPtr;
}

void StackAllocator::Deallocate(void* ptr)
{
    MEMORY_MANAGER_ASSERT(ptr, "Error: Cannot deallocate nullptr!\n");

    const UIntPtr currentAddress = reinterpret_cast<UIntPtr>(ptr);

    // Check if this allocator owns the pointer
    MEMORY_MANAGER_ASSERT(OwnsAddress(currentAddress), "Error: The allocator %s does not own the pointer %d!\n", m_Data->debugName.c_str(),
                          currentAddress);

    Header* header = GetHeader<Header>(currentAddress);

    const Offset currentOffset = currentAddress - m_StartAddress;

    SetCurrentOffset(currentOffset - header->padding);
}

void* StackAllocator::AllocateArray(const Size objectCount, const Size objectSize, const Alignment& alignment)
{

    const Size allocationSize = objectCount * objectSize;

    const UIntPtr baseAddress = m_StartAddress + m_CurrentOffset;

    // The padding includes alignment as well as the header
    const Padding padding = CalculateAlignedPaddingWithHeader(baseAddress, alignment, sizeof(ArrayHeader));

    const Size totalSizeAfterAllocation = m_CurrentOffset + padding + allocationSize;

    // Check if this allocation will overflow the stack allocator
    MEMORY_MANAGER_ASSERT(totalSizeAfterAllocation <= m_Data->totalSize, "Error: The allocator %s is out of memory!\n",
                          m_Data->debugName.c_str());

    const UIntPtr alignedAddress = baseAddress + padding;
    AllocateHeader<ArrayHeader>(alignedAddress, padding, objectCount);

    SetCurrentOffset(totalSizeAfterAllocation);

    void* allocatedPtr = reinterpret_cast<void*>(alignedAddress);

    return allocatedPtr;
}

Size StackAllocator::DeallocateArray(void* ptr)
{
    MEMORY_MANAGER_ASSERT(ptr, "Error: Cannot deallocate nullptr!\n");

    const UIntPtr currentAddress = reinterpret_cast<UIntPtr>(ptr);

    // Check if this allocator owns the pointer
    MEMORY_MANAGER_ASSERT(OwnsAddress(currentAddress), "Error: The allocator %s does not own the pointer %d!\n", m_Data->debugName.c_str(),
                          currentAddress);

    ArrayHeader* header = GetHeader<ArrayHeader>(currentAddress);

    const Offset currentOffset = currentAddress - m_StartAddress;

    SetCurrentOffset(currentOffset - header->padding);

    return header->count;
}

} // namespace Memarena