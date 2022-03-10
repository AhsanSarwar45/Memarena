#pragma once

#include "Source/AllocatorData.hpp"
#include "Source/Assert.hpp"
#include "Source/Utility/Alignment.hpp"
#include "StackAllocatorBase.hpp"

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
template <StackAllocatorPolicy allocatorPolicy = StackAllocatorPolicy()>
class StackAllocator : public Internal::StackAllocatorBase
{
  private:
    using HeaderBase = typename std::conditional<allocatorPolicy.stackCheckPolicy == StackCheckPolicy::None, Internal::UnsafeHeaderBase,
                                                 Internal::SafeHeaderBase>::type;

    struct InplaceHeader : public HeaderBase
    {
        Offset startOffset;

        InplaceHeader(Offset _startOffset, Offset _endOffset) : HeaderBase(_endOffset), startOffset(_startOffset) {}
    };
    using Header             = Internal::StackHeader;
    using InplaceArrayHeader = Internal::StackArrayHeader;
    using ArrayHeader        = Internal::StackArrayHeader;

  public:
    // Prohibit default construction, moving and assignment
    StackAllocator()                      = delete;
    StackAllocator(const StackAllocator&) = delete;
    StackAllocator(StackAllocator&&)      = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;
    StackAllocator& operator=(StackAllocator&&) = delete;

    explicit StackAllocator(const Size totalSize, const std::shared_ptr<MemoryManager> memoryManager = nullptr,
                            const std::string& debugName = "StackAllocator")
        : Internal::StackAllocatorBase(totalSize, memoryManager, debugName)
    {
    }

    template <typename Object, typename... Args>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] StackPtr<Object> New(Args&&... argList)
    {
        Offset  startOffset = m_CurrentOffset;
        void*   voidPtr     = AllocateInternal<0>(sizeof(Object), AlignOf(alignof(Object)));
        Offset  endOffset   = m_CurrentOffset;
        Object* objectPtr   = new (voidPtr) Object(std::forward<Args>(argList)...);
        return StackPtr<Object>(objectPtr, Header(startOffset, endOffset));
    }

    template <typename Object, typename... Args>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] Object* NewRaw(Args&&... argList)
    {
        void*   voidPtr   = Allocate<Object>();
        Object* objectPtr = new (voidPtr) Object(std::forward<Args>(argList)...);
        return objectPtr;
    }

    template <typename Object>
    void Delete(StackPtr<Object> ptr)
    {
        Deallocate(StackPtr<void>(ptr.ptr, ptr.header));
        ptr->~Object();
    }

    template <typename Object>
    void Delete(Object* ptr)
    {
        Deallocate(ptr);
        ptr->~Object();
    }

    template <typename Object, typename... Args>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] StackArrayPtr<Object> NewArray(const Size objectCount, Args&&... argList)
    {
        Offset startOffset = m_CurrentOffset;
        void*  voidPtr     = AllocateInternal<0>(objectCount * sizeof(Object), AlignOf(alignof(Object)));
        return StackArrayPtr<Object>(Internal::ConstructArray<Object>(voidPtr, objectCount, std::forward<Args>(argList)...),
                                     ArrayHeader(startOffset, objectCount));
    }

    template <typename Object, typename... Args>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] Object* NewArrayRaw(const Size objectCount, Args&&... argList)
    {
        void* voidPtr = AllocateArray<Object>(objectCount);
        return Internal::ConstructArray<Object>(voidPtr, objectCount, std::forward<Args>(argList)...);
    }

    template <typename Object>
    void DeleteArray(Object* ptr)
    {
        Size objectCount = DeallocateArray(ptr, sizeof(Object));
        Internal::DestructArray(ptr, objectCount);
    }

    template <typename Object>
    void DeleteArray(StackArrayPtr<Object> ptr)
    {
        Size objectCount = DeallocateArray(StackArrayPtr<void>(ptr.ptr, ptr.header), sizeof(Object));
        Internal::DestructArray(ptr.GetPtr(), objectCount);
    }

    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* Allocate(const Size size, const Alignment& alignment)
    {
        const Offset startOffset = m_CurrentOffset;
        void*        ptr         = AllocateInternal<sizeof(InplaceHeader)>(size, alignment);
        const Offset endOffset   = m_CurrentOffset;
        Internal::AllocateHeader<InplaceHeader>(ptr, startOffset, endOffset);
        return ptr;
    }

    template <typename Object>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* Allocate()
    {
        return Allocate(sizeof(Object), AlignOf(alignof(Object)));
    }

    void Deallocate(void* ptr)
    {
        const UIntPtr currentAddress = GetAddressFromPtr(ptr);
        UIntPtr       addressMarker  = currentAddress;
        InplaceHeader header         = Internal::GetHeaderFromPtr<InplaceHeader>(addressMarker);
        DeallocateInternal(currentAddress, addressMarker, header);
    }

    void Deallocate(const StackPtr<void>& ptr)
    {
        const UIntPtr currentAddress = GetAddressFromPtr(ptr.GetPtr());
        DeallocateInternal(currentAddress, currentAddress, ptr.header);
    }

    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* AllocateArray(const Size objectCount, const Size objectSize, const Alignment& alignment)
    {
        const Size   allocationSize = objectCount * objectSize;
        const Offset startOffset    = m_CurrentOffset;
        void*        ptr            = AllocateInternal<sizeof(InplaceArrayHeader)>(allocationSize, alignment);
        Internal::AllocateHeader<InplaceArrayHeader>(ptr, startOffset, objectCount);
        return ptr;
    }

    template <typename Object>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* AllocateArray(const Size objectCount)
    {
        return AllocateArray(objectCount, sizeof(Object), AlignOf(alignof(Object)));
    }

    Size DeallocateArray(void* ptr, const Size objectSize)
    {
        const UIntPtr            currentAddress = GetAddressFromPtr(ptr);
        UIntPtr                  addressMarker  = currentAddress;
        const InplaceArrayHeader header         = Internal::GetHeaderFromPtr<InplaceArrayHeader>(addressMarker);
        DeallocateInternal(
            currentAddress, addressMarker,
            Header(header.startOffset, Internal::GetArrayEndOffset(currentAddress, m_StartAddress, header.count, objectSize)));
        return header.count;
    }

    Size DeallocateArray(const StackArrayPtr<void>& ptr, const Size objectSize)
    {
        const UIntPtr currentAddress = GetAddressFromPtr(ptr.GetPtr());
        DeallocateInternal(
            currentAddress, currentAddress,
            Header(ptr.header.startOffset, Internal::GetArrayEndOffset(currentAddress, m_StartAddress, ptr.header.count, objectSize)));
        return ptr.header.count;
    }

  private:
    StackAllocator(StackAllocator& stackAllocator); // Restrict copying

    template <Size headerSize>
    void* AllocateInternal(const Size size, const Alignment& alignment)
    {
        const UIntPtr baseAddress = m_StartAddress + m_CurrentOffset;

        Padding padding;
        UIntPtr alignedAddress;

        constexpr Size totalHeaderSize = Internal::GetTotalHeaderSize<headerSize, allocatorPolicy>();

        if constexpr (totalHeaderSize > 0)
        {
            padding        = CalculateAlignedPaddingWithHeader(baseAddress, alignment, totalHeaderSize);
            alignedAddress = baseAddress + padding;
        }
        else
        {
            alignedAddress = CalculateAlignedAddress(baseAddress, alignment);
            padding        = alignedAddress - baseAddress;
        }

        Size totalSizeAfterAllocation = m_CurrentOffset + padding + size;

        if constexpr (allocatorPolicy.sizeCheckPolicy == SizeCheckPolicy::Check)
        {
            MEMARENA_ASSERT(totalSizeAfterAllocation <= m_Data->totalSize, "Error: The allocator %s is out of memory!\n",
                            m_Data->debugName.c_str());
        }

        if constexpr (allocatorPolicy.boundsCheckPolicy == BoundsCheckPolicy::Basic)
        {
            totalSizeAfterAllocation += sizeof(BoundGuardBack);

            const UIntPtr frontGuardAddress = alignedAddress - totalHeaderSize;
            const UIntPtr backGuardAddress  = alignedAddress + size;

            new (reinterpret_cast<void*>(frontGuardAddress)) BoundGuardFront(m_CurrentOffset, size);
            new (reinterpret_cast<void*>(backGuardAddress)) BoundGuardBack(m_CurrentOffset);
        }

        SetCurrentOffset(totalSizeAfterAllocation);

        void* allocatedPtr = reinterpret_cast<void*>(alignedAddress);
        return allocatedPtr;
    }

    template <typename Header>
    void DeallocateInternal(const UIntPtr address, const UIntPtr addressMarker, const Header& header)
    {
        const Offset newOffset = header.startOffset;

        if constexpr (allocatorPolicy.stackCheckPolicy == StackCheckPolicy::Check)
        {
            MEMARENA_ASSERT(header.endOffset == m_CurrentOffset, "Error: Attempt to deallocate in wrong order in the stack allocator %s!\n",
                            m_Data->debugName.c_str());
        }

        if constexpr (allocatorPolicy.boundsCheckPolicy == BoundsCheckPolicy::Basic)
        {
            const UIntPtr          frontGuardAddress = addressMarker - sizeof(BoundGuardFront);
            const BoundGuardFront* frontGuard        = reinterpret_cast<BoundGuardFront*>(frontGuardAddress);

            const UIntPtr         backGuardAddress = address + frontGuard->allocationSize;
            const BoundGuardBack* backGuard        = reinterpret_cast<BoundGuardBack*>(backGuardAddress);

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
            MEMARENA_ASSERT(OwnsAddress(address), "Error: The allocator %s does not own the pointer %d!\n", m_Data->debugName.c_str(),
                            address);
        }

        return address;
    }
};
} // namespace Memarena