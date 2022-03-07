#pragma once

#include "Source/AllocatorData.hpp"
#include "Source/Assert.hpp"
#include "Source/Policies.hpp"
#include "Source/Utility/Alignment.hpp"
#include "StackAllocatorBase.hpp"

namespace Memarena
{
template <Size headerSize, StackAllocatorPolicy allocatorPolicy>
consteval Size GetTotalHeaderSize()
{
    if constexpr (allocatorPolicy.boundsCheckPolicy == BoundsCheckPolicy::Basic)
    {
        return headerSize + sizeof(BoundGuardFront);
    }
    else if constexpr (allocatorPolicy.boundsCheckPolicy == BoundsCheckPolicy::None)
    {
        return headerSize;
    }
}

struct StackHeader
{
    Offset startOffset;
    Offset endOffset;

    StackHeader(Offset _startOffset, Offset _endOffset) : startOffset(_startOffset), endOffset(_endOffset) {}
};

template <typename T>
struct Ptr
{

  public:
    Ptr(T* _ptr) : ptr(_ptr) {}

    inline T* GetPtr() const { return ptr; }

    T*       operator->() const { return ptr; }
    explicit operator bool() const noexcept { return (ptr != nullptr); }
    T        operator[](int index) const { return ptr[index]; }

  protected:
    T* ptr;
};

template <typename T>
struct StackPtr : public Ptr<T>
{
  public:
    StackHeader header;

    StackPtr(T* _ptr, StackHeader _header) : Ptr<T>(_ptr), header(_header) {}
    operator StackPtr<void>() const noexcept { return StackPtr<void>(this->ptr, header); }
};

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
template <StackAllocatorPolicy allocatorPolicy = StackAllocatorPolicy()>
class StackAllocator : public StackAllocatorBase
{
  private:
    struct SafeHeaderBase
    {
        Offset endOffset;

        SafeHeaderBase(Offset _endOffset) : endOffset(_endOffset) {}
    };
    struct UnsafeHeaderBase
    {
        UnsafeHeaderBase(Offset _endOffset) {}
    };

    using HeaderBase = std::conditional<allocatorPolicy.stackCheckPolicy == StackCheckPolicy::None, UnsafeHeaderBase, SafeHeaderBase>::type;

    struct InplaceHeader : public HeaderBase
    {
        Offset startOffset;

        InplaceHeader(Offset _startOffset, Offset _endOffset) : HeaderBase(_endOffset), startOffset(_startOffset) {}
    };

    using InplaceArrayHeader = StackHeader;

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
    StackPtr<Object> New(Args&&... argList)
    {
        Offset startOffset = m_CurrentOffset;
        void*  voidPtr     = AllocateInternalObject<Object>();
        Offset endOffset   = m_CurrentOffset;
        return StackPtr<Object>(ConstructObject<Object>(voidPtr, argList...), StackHeader(startOffset, endOffset));
    }

    template <typename Object, typename... Args>
    Object* NewRaw(Args&&... argList)
    {
        void* voidPtr = Allocate<Object>(); // Allocate the raw memory and get a pointer to it
        return ConstructObject<Object>(voidPtr, argList...);
    }

    /**
     * @brief Deallocates a pointer and calls the destructor
     * @details Speed complexity is O(1)
     *
     * @tparam Object The type of the passed pointer
     * @param ptr The pointer to the memory to be deallocated
     */
    template <typename Object>
    void Delete(StackPtr<Object> ptr)
    {
        Deallocate(ptr); // Deallocate the pointer

        ptr->~Object(); // Call the destructor on the object
    }

    template <typename Object>
    void Delete(Object* ptr)
    {
        Deallocate(ptr); // Deallocate the pointer

        ptr->~Object(); // Call the destructor on the object
    }

    template <typename Object, typename... Args>
    StackPtr<Object> NewArray(const Size objectCount, Args... argList)
    {
        Offset startOffset = m_CurrentOffset;
        void*  voidPtr     = AllocateInternalArray<Object>(objectCount);
        Offset endOffset   = m_CurrentOffset;
        return StackPtr<Object>(ConstructArray<Object>(voidPtr, objectCount, argList...), StackHeader(startOffset, endOffset));
    }

    template <typename Object, typename... Args>
    Object* NewArrayRaw(const Size objectCount, Args... argList)
    {
        // Allocate the raw memory and get a pointer to it
        void* voidPtr = AllocateArray<Object>(objectCount);
        return ConstructArray<Object>(voidPtr, objectCount, argList...);
    }

    // template <typename Object, typename... Args>
    // StackArrayPtr<Object> NewArrayRaw(const Size objectCount)
    // {
    //     return static_cast<Object*>(AllocateArray<Object>(objectCount));
    // }

    template <typename Object>
    void DeleteArray(Object* ptr)
    {
        Size objectCount = DeallocateArray(ptr, sizeof(Object));
        DestructArray(ptr, objectCount);
    }

    template <typename Object>
    void DeleteArray(StackPtr<Object> ptr)
    {
        Size objectCount = DeallocateArray(ptr, sizeof(Object));
        DestructArray(ptr.GetPtr(), objectCount);
    }

    /**
     * @brief Allocates raw memory without calling any constructor
     * @details Speed complexity is O(1)
     * @param size The size of the memory to be allocated in bytes
     * @param alignment The alignment of the memory to be allocated in bytes
     * @return void* The pointer to the newly allocated memory
     */

    void* Allocate(const Size size, const Alignment& alignment) { return AllocateWithHeader<InplaceHeader>(size, alignment); }

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
    void Deallocate(void* ptr) { DeallocateInternal(ptr); }
    void Deallocate(StackPtr<void> ptr) { DeallocateInternal(ptr); }

    void* AllocateArray(const Size objectCount, const Size objectSize, const Alignment& alignment)
    {
        const Size allocationSize = objectCount * objectSize;

        return AllocateWithHeader<InplaceArrayHeader>(allocationSize, alignment);
    }

    template <typename Object>
    void* AllocateArray(const Size objectCount)
    {
        return AllocateArray(objectCount, sizeof(Object), AlignOf(alignof(Object)));
    }

    Size DeallocateArray(void* ptr, const Size objectSize)
    {
        const UIntPtr currentAddress = GetAddressFromPtr(ptr);

        UIntPtr                  addressMarker = currentAddress;
        const InplaceArrayHeader header        = GetHeaderFromPtr<InplaceArrayHeader>(addressMarker);

        DeallocateInternal<InplaceArrayHeader>(currentAddress, addressMarker, header);

        const Size numObjects = GetArrayCount(currentAddress, header.endOffset, objectSize);
        return numObjects;
    }

    Size DeallocateArray(StackPtr<void> ptr, const Size objectSize)
    {
        const UIntPtr currentAddress = GetAddressFromPtr(ptr.GetPtr());

        DeallocateInternal<StackHeader>(currentAddress, currentAddress, ptr.header);

        const Size numElements = GetArrayCount(currentAddress, ptr.header.endOffset, objectSize);
        return numElements;
    }

  private:
    StackAllocator(StackAllocator& stackAllocator); // Restrict copying

    template <typename Header>
    void* AllocateWithHeader(const Size size, const Alignment alignment)
    {
        constexpr Size headerSize = sizeof(Header);

        const Offset startOffset = m_CurrentOffset;
        void*        ptr         = AllocateInternal<headerSize>(size, alignment);
        const Offset endOffset   = m_CurrentOffset;

        AllocateHeader<Header>(ptr, startOffset, endOffset);

        return ptr;
    }

    template <typename Object>
    void* AllocateInternalObject()
    {
        return AllocateInternal<0>(sizeof(Object), AlignOf(alignof(Object)));
    }

    template <typename Object>
    void* AllocateInternalArray(const Size objectCount)
    {
        return AllocateInternal<0>(objectCount * sizeof(Object), AlignOf(alignof(Object)));
    }

    template <Size headerSize, typename... Args>
    void* AllocateInternal(const Size size, const Alignment& alignment)
    {
        const UIntPtr baseAddress = m_StartAddress + m_CurrentOffset;

        Padding padding;
        UIntPtr alignedAddress;

        constexpr Size totalHeaderSize = GetTotalHeaderSize<headerSize, allocatorPolicy>();

        if constexpr (totalHeaderSize > 0)
        { // The padding includes alignment as well as the header
            padding        = CalculateAlignedPaddingWithHeader(baseAddress, alignment, totalHeaderSize);
            alignedAddress = baseAddress + padding;
        }
        else
        {
            alignedAddress = CalculateAlignedAddress(baseAddress, alignment);
            padding        = alignedAddress - baseAddress;
        }

        const Size totalSizeAfterAllocation = m_CurrentOffset + padding + size;

        if constexpr (allocatorPolicy.sizeCheckPolicy == SizeCheckPolicy::Check)
        {
            // Check if this allocation will overflow the stack allocator
            MEMARENA_ASSERT(totalSizeAfterAllocation <= m_Data->totalSize, "Error: The allocator %s is out of memory!\n",
                            m_Data->debugName.c_str());
        }

        if constexpr (allocatorPolicy.boundsCheckPolicy == BoundsCheckPolicy::Basic)
        {
            const UIntPtr frontGuardAddress = alignedAddress - totalHeaderSize;
            const UIntPtr backGuardAddress  = alignedAddress + size;

            new (reinterpret_cast<void*>(frontGuardAddress)) BoundGuardFront(m_CurrentOffset, size);
            new (reinterpret_cast<void*>(backGuardAddress)) BoundGuardBack(m_CurrentOffset);
        }

        SetCurrentOffset(totalSizeAfterAllocation);

        void* allocatedPtr = allocatedPtr = reinterpret_cast<void*>(alignedAddress);

        return allocatedPtr;
    }

    void DeallocateInternal(void* ptr)
    {
        const UIntPtr currentAddress = GetAddressFromPtr(ptr);
        UIntPtr       addressMarker  = currentAddress;

        InplaceHeader header = GetHeaderFromPtr<InplaceHeader>(addressMarker);

        DeallocateInternal<InplaceHeader>(currentAddress, addressMarker, header);
    }

    void DeallocateInternal(StackPtr<void> ptr)
    {
        const UIntPtr currentAddress = GetAddressFromPtr(ptr.GetPtr());

        DeallocateInternal<StackHeader>(currentAddress, currentAddress, ptr.header);
    }

    template <typename Header>
    void DeallocateInternal(const UIntPtr address, const UIntPtr addressMarker, const Header& header)
    {
        if constexpr (allocatorPolicy.stackCheckPolicy == StackCheckPolicy::Check)
        {
            MEMARENA_ASSERT(header.endOffset == m_CurrentOffset, "Error: Attempt to deallocate in wrong order in the stack allocator %s!\n",
                            m_Data->debugName.c_str());
        }

        const Offset newOffset = header.startOffset;

        if constexpr (allocatorPolicy.boundsCheckPolicy == BoundsCheckPolicy::Basic)
        {
            const UIntPtr          frontGuardAddress = addressMarker - sizeof(BoundGuardFront);
            const BoundGuardFront* frontGuard        = reinterpret_cast<BoundGuardFront*>(frontGuardAddress);

            const UIntPtr         backGuardAddress = frontGuardAddress + frontGuard->allocationSize;
            const BoundGuardBack* backGuard        = reinterpret_cast<BoundGuardBack*>(frontGuardAddress);

            MEMARENA_ASSERT(frontGuard->offset == newOffset && backGuard->offset == newOffset,
                            "Error: Memory stomping detected in allocator %s at offset %d and address %d!\n", m_Data->debugName.c_str(),
                            newOffset, address);
        }

        SetCurrentOffset(newOffset);
    }

    UIntPtr GetAddressFromPtr(void* ptr)
    {
        if constexpr (allocatorPolicy.nullCheckPolicy == NullCheckPolicy::Check)
        {
            MEMARENA_ASSERT(ptr, "Error: Cannot deallocate nullptr!\n");
        }

        const UIntPtr address = reinterpret_cast<UIntPtr>(ptr);

        if constexpr (allocatorPolicy.ownershipCheckPolicy == OwnershipCheckPolicy::Check)
        {
            // Check if this allocator owns the pointer
            MEMARENA_ASSERT(OwnsAddress(address), "Error: The allocator %s does not own the pointer %d!\n", m_Data->debugName.c_str(),
                            address);
        }

        return address;
    }

    template <typename Header>
    Header GetHeaderFromPtr(UIntPtr& address)
    {
        const UIntPtr headerAddress = address - sizeof(Header);
        const Header* headerPtr     = reinterpret_cast<Header*>(headerAddress);
        address                     = headerAddress;

        return *headerPtr;
    }

    template <typename Header>
    void AllocateHeader(void* ptr, const Offset startOffset, const Offset endOffset)
    {
        const UIntPtr address = reinterpret_cast<UIntPtr>(ptr);

        const UIntPtr headerAddress = address - sizeof(Header);
        // Construct the header at 'headerAdress' using placement new operator
        void* headerPtr = reinterpret_cast<void*>(headerAddress);
        new (headerPtr) Header(startOffset, endOffset);
    }

    template <typename Object, typename... Args>
    Object* ConstructObject(void* voidPtr, Args... argList)
    {
        return new (voidPtr) Object(argList...);
    }
    template <typename Object, typename... Args>
    Object* ConstructArray(void* voidPtr, const Offset objectCount, Args... argList)
    {
        // Call the placement new operator, which constructs the Object
        Object* firstPtr = new (voidPtr) Object(argList...);

        Object* currentPtr = firstPtr;
        Object* lastPtr    = firstPtr + (objectCount - 1);

        while (currentPtr <= lastPtr)
        {
            new (currentPtr++) Object(argList...);
        }

        return firstPtr;
    }

    Offset GetArrayCount(const UIntPtr ptrAddress, const Offset endOffset, const Size objectSize)
    {
        const Offset addressOffset = ptrAddress - m_StartAddress;
        return (endOffset - addressOffset) / objectSize;
    }

    template <typename Object>
    void DestructArray(Object* ptr, const Offset objectCount)
    {
        for (Size i = objectCount - 1; i-- > 0;)
        {
            ptr[i].~Object();
        }
    }
};

} // namespace Memarena