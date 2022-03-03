#pragma once

#include "Source/AllocatorData.hpp"
#include "Source/Assert.hpp"
#include "Source/BoundsChecking.hpp"
#include "Source/Utility/Alignment.hpp"
#include "StackAllocatorBase.hpp"

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
 */
template <typename BoundsCheckingPolicy = NoBoundsChecking>
class StackAllocator : public StackAllocatorBase
{
  public:
    // Prohibit default construction, moving and assignment
    StackAllocator()                      = delete;
    StackAllocator(const StackAllocator&) = delete;
    StackAllocator(StackAllocator&&)      = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;
    StackAllocator& operator=(StackAllocator&&) = delete;

    StackAllocator(const Size totalSize, const std::shared_ptr<MemoryManager> memoryManager = nullptr,
                   const std::string& debugName = "StackAllocator")
        : StackAllocatorBase(totalSize, memoryManager, debugName)
    {
    }

    /**
     * @brief Allocates a new block of memory and calls the constructor
     * @details Speed complexity is O(1)
     *
     * @tparam Object The type to be created
     * @tparam Args Variadic arguments
     * @param argList The arguments to the constructor of the type "Object"
     * @return Object* The pointer to the newly allocated and created object
     */
    template <typename Object, typename... Args>
    Object* New(Args&&... argList)
    {
        void* rawPtr = Allocate<Object>();      // Allocate the raw memory and get a pointer to it
        return new (rawPtr) Object(argList...); // Call the placement new operator, which constructs the Object
    }

    /**
     * @brief Deallocates a pointer and calls the destructor
     * @details Speed complexity is O(1)
     *
     * @tparam Object The type of the passed pointer
     * @param ptr The pointer to the memory to be deallocated
     */
    template <typename Object>
    void Delete(Object* ptr)
    {
        Deallocate(ptr); // Deallocate the pointer

        ptr->~Object(); // Call the destructor on the object
    }

    template <typename Object, typename... Args>
    Object* NewArray(const Size objectCount, Args... argList)
    {
        // Allocate the raw memory and get a pointer to it
        void* rawPtr = AllocateArray<Object>(objectCount);
        // Call the placement new operator, which constructs the Object
        Object* firstPtr = new (rawPtr) Object(argList...);

        Object* currentPtr = firstPtr;
        Object* lastPtr    = firstPtr + (objectCount - 1);

        while (currentPtr <= lastPtr)
        {
            new (currentPtr++) Object(argList...);
        }

        return firstPtr;
    }

    template <typename Object, typename... Args>
    Object* NewArrayRaw(const Size objectCount)
    {
        return static_cast<Object*>(AllocateArray<Object>(objectCount));
    }

    template <typename Object>
    void DeleteArray(Object* ptr)
    {
        Size objectCount = DeallocateArray(ptr);

        for (Size i = objectCount - 1; i-- > 0;)
        {
            ptr[i].~Object();
        }
    }

    /**
     * @brief Allocates raw memory without calling any constructor
     * @details Speed complexity is O(1)
     * @param size The size of the memory to be allocated in bytes
     * @param alignment The alignment of the memory to be allocated in bytes
     * @return void* The pointer to the newly allocated memory
     */
    void* Allocate(const Size size, const Alignment& alignment)
    {
        const UIntPtr baseAddress = m_StartAddress + m_CurrentOffset;

        // The padding includes alignment as well as the header
        const Padding padding =
            CalculateAlignedPaddingWithHeader(baseAddress, alignment, sizeof(Header) + BoundsCheckingPolicy::s_FrontGuardSize);

        const Size totalSizeAfterAllocation = m_CurrentOffset + padding + size;
        // Check if this allocation will overflow the stack allocator
        MEMARENA_ASSERT(totalSizeAfterAllocation <= m_Data->totalSize, "Error: The allocator %s is out of memory!\n",
                        m_Data->debugName.c_str());

        const UIntPtr alignedAddress = baseAddress + padding;

        const UIntPtr headerAddress = alignedAddress - sizeof(Header);
        AllocateHeader<Header>(headerAddress, padding);

        m_BoundsChecker.Guard(headerAddress, m_CurrentOffset, size);

        SetCurrentOffset(totalSizeAfterAllocation);

        void* allocatedPtr = reinterpret_cast<void*>(alignedAddress);
        return allocatedPtr;
    }

    template <typename Object>
    void* Allocate()
    {
        return Allocate(sizeof(Object), AlignOf(alignof(Object)));
    }

    /**
     * @brief Deallocates raw memory without calling any destructor. It also deallocates
     * all allocations that were done after this pointer was allocated.
     * @details Speed complexity is O(1)
     * @param ptr The pointer to the memory to be deallocated
     */
    void Deallocate(void* ptr)
    {
        MEMARENA_ASSERT(ptr, "Error: Cannot deallocate nullptr!\n");

        const UIntPtr currentAddress = reinterpret_cast<UIntPtr>(ptr);

        // Check if this allocator owns the pointer
        MEMARENA_ASSERT(OwnsAddress(currentAddress), "Error: The allocator %s does not own the pointer %d!\n", m_Data->debugName.c_str(),
                        currentAddress);

        const UIntPtr headerAddress = currentAddress - sizeof(Header);
        Header*       header        = GetHeader<Header>(headerAddress);

        const Offset currentOffset = currentAddress - m_StartAddress;
        const Offset newOffset     = currentOffset - header->padding;

        m_BoundsChecker.Check(headerAddress, newOffset, m_Data->debugName.c_str());

        SetCurrentOffset(newOffset);
    }

    void* AllocateArray(const Size objectCount, const Size objectSize, const Alignment& alignment)
    {
        const Size allocationSize = objectCount * objectSize;

        const UIntPtr baseAddress = m_StartAddress + m_CurrentOffset;

        // The padding includes alignment as well as the header
        const Padding padding = CalculateAlignedPaddingWithHeader(baseAddress, alignment, sizeof(ArrayHeader));

        const Size totalSizeAfterAllocation = m_CurrentOffset + padding + allocationSize;

        // Check if this allocation will overflow the stack allocator
        MEMARENA_ASSERT(totalSizeAfterAllocation <= m_Data->totalSize, "Error: The allocator %s is out of memory!\n",
                        m_Data->debugName.c_str());

        const UIntPtr alignedAddress = baseAddress + padding;

        const UIntPtr headerAddress = alignedAddress - sizeof(ArrayHeader);
        AllocateHeader<ArrayHeader>(headerAddress, padding, objectCount);

        m_BoundsChecker.Guard(headerAddress, m_CurrentOffset, allocationSize);

        SetCurrentOffset(totalSizeAfterAllocation);

        void* allocatedPtr = reinterpret_cast<void*>(alignedAddress);

        return allocatedPtr;
    }

    template <typename Object>
    void* AllocateArray(const Size objectCount)
    {
        return AllocateArray(objectCount, sizeof(Object), AlignOf(alignof(Object)));
    }

    Size DeallocateArray(void* ptr)
    {
        MEMARENA_ASSERT(ptr, "Error: Cannot deallocate nullptr!\n");

        const UIntPtr currentAddress = reinterpret_cast<UIntPtr>(ptr);

        // Check if this allocator owns the pointer
        MEMARENA_ASSERT(OwnsAddress(currentAddress), "Error: The allocator %s does not own the pointer %d!\n", m_Data->debugName.c_str(),
                        currentAddress);

        const UIntPtr headerAddress = currentAddress - sizeof(ArrayHeader);
        ArrayHeader*  header        = GetHeader<ArrayHeader>(headerAddress);

        const Offset currentOffset = currentAddress - m_StartAddress;
        const Offset newOffset     = currentOffset - header->padding;

        m_BoundsChecker.Check(headerAddress, newOffset, m_Data->debugName.c_str());

        SetCurrentOffset(newOffset);

        return header->count;
    }

  private:
    StackAllocator(StackAllocator& stackAllocator); // Restrict copying

    template <typename Header, typename... Args>
    void AllocateHeader(const UIntPtr address, Args... argList)
    {
        // Construct the header at 'headerAdress' using placement new operator
        void* headerPtr = reinterpret_cast<void*>(address);
        new (headerPtr) Header(argList...);
    }

    template <typename Header>
    Header* GetHeader(const UIntPtr address)
    {
        return reinterpret_cast<Header*>(address);
    }

    struct Header
    {
        Padding padding;

        Header(Padding _padding) : padding(_padding) {}
    };
    struct ArrayHeader
    {
        Offset  count;
        Padding padding;

        ArrayHeader(Padding _padding, Size _count) : count(_count), padding(_padding) {}
    };

  private:
    BoundsCheckingPolicy m_BoundsChecker;
};

} // namespace Memarena