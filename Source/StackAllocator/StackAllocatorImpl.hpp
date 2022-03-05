#pragma once

#include "Source/TypeAliases.hpp"

namespace Memarena
{

struct AllocationHeaderUnsafe
{
    Padding padding;

    AllocationHeaderUnsafe(Padding _padding) : padding(_padding) {}
};
struct AllocationHeaderSafe
{
    Offset  endOffset;
    Padding padding;

    AllocationHeaderSafe(Offset _endOffset, Padding _padding) : endOffset(_endOffset), padding(_padding) {}
};

struct ArrayHeaderUnsafe
{
    Offset  count;
    Offset  endOffset;
    Padding padding;

    ArrayHeader(Padding _padding, Offset _endOffset, Offset _count) : count(_count), endOffset(_endOffset), padding(_padding) {}
};

struct ArrayHeaderSafe
{
    Offset  count;
    Padding padding;

    ArrayHeader(Padding _padding, Size _count) : count(_count), padding(_padding) {}
};

void* Allocate(const UIntPtr startAddress, const Offset currentOffset, const Size size, const Alignment& alignment)
{
    const UIntPtr baseAddress = startAddress + currentOffset;

    // The padding includes alignment as well as the header
    const Padding padding = CalculateAlignedPaddingWithHeader(baseAddress, alignment, GetHeaderSize<AllocationHeader>());

    const Size totalSizeAfterAllocation = currentOffset + padding + size;
    // Check if this allocation will overflow the stack allocator
    MEMARENA_ASSERT(totalSizeAfterAllocation <= m_Data->totalSize, "Error: The allocator %s is out of memory!\n",
                    m_Data->debugName.c_str());

    const UIntPtr alignedAddress = baseAddress + padding;

    return GetAllocatedPtrWithHeader<AllocationHeader>(alignedAddress, size, totalSizeAfterAllocation, padding);
}

void* StackAllocatorAllocateArray(const UIntPtr startAddress, const Size objectCount, const Size objectSize, const Alignment& alignment)
{
    const Size allocationSize = objectCount * objectSize;

    const UIntPtr baseAddress = m_StartAddress + m_CurrentOffset;

    // The padding includes alignment as well as the header
    const Padding padding = CalculateAlignedPaddingWithHeader(baseAddress, alignment, GetHeaderSize<ArrayHeader>());

    const Size totalSizeAfterAllocation = m_CurrentOffset + padding + allocationSize;

    // Check if this allocation will overflow the stack allocator
    MEMARENA_ASSERT(totalSizeAfterAllocation <= m_Data->totalSize, "Error: The allocator %s is out of memory!\n",
                    m_Data->debugName.c_str());

    const UIntPtr alignedAddress = baseAddress + padding;

    return GetAllocatedPtrWithHeader<ArrayHeader>(alignedAddress, allocationSize, totalSizeAfterAllocation, padding, objectCount);
}

void* GetAllocatedPtr(const UIntPtr address, const UIntPtr headerAddress, const Size allocationSize, const Size totalSizeAfterAllocation)
{
    if constexpr (boundsCheckingPolicy == BoundsCheckingPolicy::None)
    {
    }
    else if constexpr (boundsCheckingPolicy == BoundsCheckingPolicy::Basic)
    {
        const UIntPtr frontGuardAddress = headerAddress - sizeof(BoundGuardFront);
        const UIntPtr backGuardAddress  = address + allocationSize;

        new (reinterpret_cast<void*>(frontGuardAddress)) BoundGuardFront(m_CurrentOffset, allocationSize);
        new (reinterpret_cast<void*>(backGuardAddress)) BoundGuardBack(m_CurrentOffset);
    }

    SetCurrentOffset(totalSizeAfterAllocation);

    void* allocatedPtr = reinterpret_cast<void*>(address);

    return allocatedPtr;
}

void* GetAllocatedPtrArraySafe(const UIntPtr address, const Size allocationSize, const Size totalSizeAfterAllocation, Padding padding,
                               Size count)
{
    const UIntPtr headerAddress = address - sizeof(ArrayHeaderSafe);
    // Construct the header at 'headerAdress' using placement new operator
    void* headerPtr = reinterpret_cast<void*>(headerAddress);
    new (headerPtr) ArrayHeaderSafe(count, padding);

    GetAllocatedPtr(address, headerAddress, allocationSize, totalSizeAfterAllocation)
}
} // namespace Memarena