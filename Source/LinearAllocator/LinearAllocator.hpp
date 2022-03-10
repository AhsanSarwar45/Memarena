#pragma once

#include "Source/AllocatorData.hpp"
#include "Source/Assert.hpp"
#include "Source/StackAllocator/StackAllocatorBase.hpp"
#include "Source/StackAllocator/StackAllocatorUtils.hpp"
#include "Source/Utility/Alignment.hpp"

#define NO_DISCARD_ALLOC_INFO "Not using the pointer returned will cause a soft memory leak!"

namespace Memarena
{

/**
 * @brief A custom memory allocator which allocates in a stack-like manner.
 * All the memory will be allocated up-front. This means it will have
 * zero allocations during runtime. This also means that it will take the same
 * amount of memory whether it is full or empty. Allocations and Deallocations
 * also need to be done in a stack-like manner. It is the responsibility of the
 * user to make sure that deallocations happen in an order that is the reverse
 * of the allocation order. If a pointer p1 that was not allocated last is deallocated,
 * future allocations will overwrite the memory of all allocations that were made
 * between the allocation and deallocation of p1.
 *
 * Space complexity is O(N*H) --> O(N) where H is the Header size and N is the number of allocations
 * Allocation and deallocation complexity: O(1)
 *
 * @tparam allocatorPolicy The StackAllocatorPolicy object to define the behaviour of this allocator
 */
template <LinearAllocatorPolicy allocatorPolicy = LinearAllocatorPolicy()>
class LinearAllocator : public Internal::StackAllocatorBase
{
  public:
    // Prohibit default construction, moving and assignment
    LinearAllocator()                       = delete;
    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator(LinearAllocator&&)      = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;
    LinearAllocator& operator=(LinearAllocator&&) = delete;

    explicit LinearAllocator(const Size totalSize, const std::shared_ptr<MemoryManager> memoryManager = nullptr,
                             const std::string& debugName = "LinearAllocator")
        : Internal::StackAllocatorBase(totalSize, memoryManager, debugName)
    {
    }

    template <typename Object, typename... Args>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] Object* NewRaw(Args&&... argList)
    {
        void* voidPtr = Allocate<Object>();
        return new (voidPtr) Object(std::forward<Args>(argList)...);
    }

    template <typename Object, typename... Args>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] Object* NewArrayRaw(const Size objectCount, Args&&... argList)
    {
        void* voidPtr = AllocateArray<Object>(objectCount);
        return Internal::ConstructArray<Object>(voidPtr, objectCount, std::forward<Args>(argList)...);
    }

    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* Allocate(const Size size, const Alignment& alignment)
    {
        const UIntPtr baseAddress = m_StartAddress + m_CurrentOffset;

        UIntPtr alignedAddress = CalculateAlignedAddress(baseAddress, alignment);
        Padding padding        = alignedAddress - baseAddress;

        Size totalSizeAfterAllocation = m_CurrentOffset + padding + size;

        if constexpr (allocatorPolicy.sizeCheckPolicy == SizeCheckPolicy::Check)
        {
            MEMARENA_ASSERT(totalSizeAfterAllocation <= m_Data->totalSize, "Error: The allocator %s is out of memory!\n",
                            m_Data->debugName.c_str());
        }

        SetCurrentOffset(totalSizeAfterAllocation);

        return reinterpret_cast<void*>(alignedAddress);
    }

    template <typename Object>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* Allocate()
    {
        return Allocate(sizeof(Object), AlignOf(alignof(Object)));
    }

    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* AllocateArray(const Size objectCount, const Size objectSize, const Alignment& alignment)
    {
        const Size allocationSize = objectCount * objectSize;
        return Allocate(allocationSize, alignment);
    }

    template <typename Object>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* AllocateArray(const Size objectCount)
    {
        return AllocateArray(objectCount, sizeof(Object), AlignOf(alignof(Object)));
    }

  private:
    LinearAllocator(LinearAllocator& linearAllocator); // Restrict copying
};
} // namespace Memarena