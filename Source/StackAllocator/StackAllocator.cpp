#include "StackAllocator.hpp"

#include "AllocatorData.hpp"
#include "Utility/Padding.hpp"

#include "Assert.hpp"

namespace Memory
{

StackAllocator::StackAllocator(const Size totalSize, const std::shared_ptr<MemoryManager> memoryManager, const Size defaultAlignment,
                               const char* debugName)
    : StackAllocatorBase(totalSize, memoryManager, defaultAlignment, debugName)
{
}

void* StackAllocator::Allocate(const Size size, const Size alignment)
{
    const UIntPtr currentAddress = m_StartAddress + m_CurrentOffset;

    // The padding includes alignment as well as the header
    const UInt8 padding = CalculatePaddingWithHeader(currentAddress, alignment, sizeof(Header));

    const Size totalSizeAfterAllocation = m_CurrentOffset + padding + size;

    // Check if this allocation will overflow the stack allocator
    MEMORY_MANAGER_ASSERT(totalSizeAfterAllocation <= m_Data->totalSize, "Error: The allocator %s is out of memory!\n", m_Data->debugName);

    const UIntPtr nextAddress   = currentAddress + padding;
    const UIntPtr headerAddress = nextAddress - sizeof(Header);
    // Construct the header at 'headerAdress' using placement new operator
    const Header* headerPtr = new (reinterpret_cast<void*>(headerAddress)) Header(padding);

    SetCurrentOffset(totalSizeAfterAllocation);

    void* allocatedPtr = reinterpret_cast<void*>(nextAddress);

    return allocatedPtr;
}

void StackAllocator::Deallocate(void* ptr)
{
    MEMORY_MANAGER_ASSERT(ptr, "Error: Cannot deallocate nullptr!\n");

    const UIntPtr currentAddress = reinterpret_cast<UIntPtr>(ptr);

    // Check if this allocator owns the pointer
    MEMORY_MANAGER_ASSERT(OwnsAddress(currentAddress), "Error: The allocator %s does not own the pointer %d!\n", m_Data->debugName,
                          currentAddress);

    const UIntPtr headerAddress = currentAddress - sizeof(Header);
    const Header* header        = reinterpret_cast<Header*>(headerAddress);

    const Size currentOffset = currentAddress - m_StartAddress;

    SetCurrentOffset(currentOffset - header->padding);
}

} // namespace Memory