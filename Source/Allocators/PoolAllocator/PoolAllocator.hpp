#pragma once

#include <experimental/source_location>

#include "Source/Allocator.hpp"
#include "Source/AllocatorData.hpp"
#include "Source/AllocatorUtils.hpp"
#include "Source/Assert.hpp"
#include "Source/Macros.hpp"
#include "Source/Policies/BoundsCheckPolicy.hpp"
#include "Source/Policies/MultithreadedPolicy.hpp"
#include "Source/Policies/Policies.hpp"
#include "Source/Utility/Alignment.hpp"

namespace Memarena
{

template <PoolAllocatorPolicy policy = PoolAllocatorPolicy::Default>
class PoolAllocator : public Allocator
{
  private:
    static constexpr bool IsPoolCheckEnabled          = PolicyContains(policy, PoolAllocatorPolicy::PoolCheck);
    static constexpr bool IsBoundsCheckEnabled        = PolicyContains(policy, PoolAllocatorPolicy::BoundsCheck);
    static constexpr bool IsNullCheckEnabled          = PolicyContains(policy, PoolAllocatorPolicy::NullCheck);
    static constexpr bool IsSizeCheckEnabled          = PolicyContains(policy, PoolAllocatorPolicy::SizeCheck);
    static constexpr bool IsOwnershipCheckEnabled     = PolicyContains(policy, PoolAllocatorPolicy::OwnershipCheck);
    static constexpr bool IsUsageTrackingEnabled      = PolicyContains(policy, PoolAllocatorPolicy::UsageTracking);
    static constexpr bool IsMultithreaded             = PolicyContains(policy, PoolAllocatorPolicy::Multithreaded);
    static constexpr bool IsAllocationTrackingEnabled = PolicyContains(policy, PoolAllocatorPolicy::AllocationTracking);

    using ThreadPolicy = MultithreadedPolicy<IsMultithreaded>;

    template <typename SyncPrimitive>
    using LockGuard = typename ThreadPolicy::template LockGuard<SyncPrimitive>;
    using Mutex     = typename ThreadPolicy::Mutex;

  public:
    // Prohibit default construction, moving and assignment
    PoolAllocator()                     = delete;
    PoolAllocator(PoolAllocator&)       = delete;
    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&&)      = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator& operator=(PoolAllocator&&) = delete;

    explicit PoolAllocator(const Size totalSize, const std::string& debugName = "PoolAllocator")
        : Allocator(totalSize, debugName), m_StartPtr(malloc(totalSize)), m_StartAddress(std::bit_cast<UIntPtr>(m_StartPtr)),
          m_EndAddress(m_StartAddress + totalSize)
    {
    }

    ~PoolAllocator() { free(m_StartPtr); };

    friend bool operator==(const PoolAllocator& s1, const PoolAllocator& s2) { return s1.m_StartAddress == s2.m_StartAddress; }

    template <typename Object, typename... Args>
    NO_DISCARD PoolPtr<Object> New(Args&&... argList)
    {
        auto [voidPtr, startOffset, endOffset] = AllocateInternal<0>(sizeof(Object), AlignOf(alignof(Object)));
        Object* objectPtr                      = new (voidPtr) Object(std::forward<Args>(argList)...);
        return PoolPtr<Object>(objectPtr, startOffset, endOffset);
    }

    template <typename Object, typename... Args>
    NO_DISCARD Object* NewRaw(Args&&... argList)
    {
        void*   voidPtr   = Allocate<Object>();
        Object* objectPtr = new (voidPtr) Object(std::forward<Args>(argList)...);
        return objectPtr;
    }

    template <typename Object>
    void Delete(PoolPtr<Object> ptr)
    {
        Deallocate(PoolPtr<void>(ptr.GetPtr(), ptr.header));
        ptr->~Object();
    }

    template <typename Object>
    void Delete(Object* ptr)
    {
        Deallocate(ptr);
        ptr->~Object();
    }

    template <typename Object, typename... Args>
    NO_DISCARD PoolArrayPtr<Object> NewArray(const Size objectCount, Args&&... argList)
    {
        auto [voidPtr, startOffset, endOffset] = AllocateInternal<0>(objectCount * sizeof(Object), AlignOf(alignof(Object)));
        return PoolArrayPtr<Object>(Internal::ConstructArray<Object>(voidPtr, objectCount, std::forward<Args>(argList)...), startOffset,
                                    objectCount);
    }

    template <typename Object, typename... Args>
    NO_DISCARD Object* NewArrayRaw(const Size objectCount, Args&&... argList)
    {
        void* voidPtr = AllocateArray<Object>(objectCount);
        return Internal::ConstructArray<Object>(voidPtr, objectCount, std::forward<Args>(argList)...);
    }

    template <typename Object>
    void DeleteArray(Object* ptr)
    {
        const Size objectCount = DeallocateArray(ptr, sizeof(Object));
        Internal::DestructArray(ptr, objectCount);
    }

    template <typename Object>
    void DeleteArray(PoolArrayPtr<Object> ptr)
    {
        const Size objectCount = DeallocateArray(PoolArrayPtr<void>(ptr.GetPtr(), ptr.header), sizeof(Object));
        Internal::DestructArray(ptr.GetPtr(), objectCount);
    }

    NO_DISCARD void* Allocate(const Size size, const Alignment& alignment, const std::string& category = "",
                              const SourceLocation& sourceLocation = SourceLocation::current())
    {
        auto [voidPtr, startOffset, endOffset] = AllocateInternal<sizeof(InplaceHeader)>(size, alignment, category, sourceLocation);
        Internal::AllocateHeader<InplaceHeader>(voidPtr, startOffset, endOffset);
        return voidPtr;
    }

    template <typename Object>
    NO_DISCARD void* Allocate(const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return Allocate(sizeof(Object), AlignOf(alignof(Object)), category, sourceLocation);
    }

    void Deallocate(void* ptr)
    {
        const UIntPtr currentAddress = GetAddressFromPtr(ptr);
        UIntPtr       addressMarker  = currentAddress;
        InplaceHeader header         = Internal::GetHeaderFromPtr<InplaceHeader>(addressMarker);
        DeallocateInternal(currentAddress, addressMarker, header);
    }

    void Deallocate(const PoolPtr<void>& ptr)
    {
        const void*   voidPtr        = ptr.GetPtr();
        const UIntPtr currentAddress = GetAddressFromPtr(voidPtr);
        DeallocateInternal(currentAddress, currentAddress, ptr.header);
    }

    NO_DISCARD void* AllocateArray(const Size objectCount, const Size objectSize, const Alignment& alignment,
                                   const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        const Size allocationSize = objectCount * objectSize;
        auto [voidPtr, startOffset, endOffset] =
            AllocateInternal<sizeof(InplaceArrayHeader)>(allocationSize, alignment, category, sourceLocation);
        Internal::AllocateHeader<InplaceArrayHeader>(voidPtr, startOffset, objectCount);
        return voidPtr;
    }

    template <typename Object>
    NO_DISCARD void* AllocateArray(const Size objectCount, const std::string& category = "",
                                   const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return AllocateArray(objectCount, sizeof(Object), AlignOf(alignof(Object)), category, sourceLocation);
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

    Size DeallocateArray(const PoolArrayPtr<void>& ptr, const Size objectSize)
    {
        const void*   voidPtr        = ptr.GetPtr();
        const UIntPtr currentAddress = GetAddressFromPtr(voidPtr);
        DeallocateInternal(
            currentAddress, currentAddress,
            Header(ptr.header.startOffset, Internal::GetArrayEndOffset(currentAddress, m_StartAddress, ptr.header.count, objectSize)));
        return ptr.header.count;
    }

    /**
     * @brief Releases the allocator to its initial state. Any further allocations
     * will possibly overwrite all object allocated prior to calling this method.
     * So make sure to only call this when you don't need any objects previously
     * allocated by this allocator.
     *
     */
    inline void Release()
    {
        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);
        SetCurrentOffset(0);
    };

  private:
    template <Size HeaderSize>
    std::tuple<void*, Offset, Offset> AllocateInternal(const Size size, const Alignment& alignment, const std::string& category = "",
                                                       const SourceLocation& sourceLocation = SourceLocation::current())
    {
        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

        const Offset  startOffset = m_CurrentOffset;
        const UIntPtr baseAddress = m_StartAddress + m_CurrentOffset;

        Padding padding{0};
        UIntPtr alignedAddress{0};

        constexpr Size totalHeaderSize = GetTotalHeaderSize<HeaderSize>();

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

        if constexpr (IsSizeCheckEnabled)
        {
            MEMARENA_ASSERT(totalSizeAfterAllocation <= GetTotalSize(), "Error: The allocator %s is out of memory!\n",
                            GetDebugName().c_str());
        }

        if constexpr (IsBoundsCheckEnabled)
        {
            totalSizeAfterAllocation += sizeof(BoundGuardBack);

            const UIntPtr frontGuardAddress = alignedAddress - totalHeaderSize;
            const UIntPtr backGuardAddress  = alignedAddress + size;

            new (std::bit_cast<void*>(frontGuardAddress)) BoundGuardFront(m_CurrentOffset, size);
            new (std::bit_cast<void*>(backGuardAddress)) BoundGuardBack(m_CurrentOffset);
        }

        SetCurrentOffset(totalSizeAfterAllocation);

        const Offset endOffset = m_CurrentOffset;

        void* allocatedPtr = std::bit_cast<void*>(alignedAddress);

        if (IsAllocationTrackingEnabled)
        {
            AddAllocation(size, category, sourceLocation);
        }

        return {allocatedPtr, startOffset, endOffset};
    }

    template <typename Header>
    void DeallocateInternal(const UIntPtr address, const UIntPtr addressMarker, const Header& header)
    {
        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

        const Offset newOffset = header.startOffset;

        if constexpr (IsPoolCheckEnabled)
        {
            MEMARENA_ASSERT(header.endOffset == m_CurrentOffset, "Error: Attempt to deallocate in wrong order in the Pool allocator %s!\n",
                            GetDebugName().c_str());
        }

        if constexpr (IsBoundsCheckEnabled)
        {
            const UIntPtr          frontGuardAddress = addressMarker - sizeof(BoundGuardFront);
            const BoundGuardFront* frontGuard        = std::bit_cast<BoundGuardFront*>(frontGuardAddress);

            const UIntPtr         backGuardAddress = address + frontGuard->allocationSize;
            const BoundGuardBack* backGuard        = std::bit_cast<BoundGuardBack*>(backGuardAddress);

            MEMARENA_ASSERT(frontGuard->offset == newOffset && backGuard->offset == newOffset,
                            "Error: Memory stomping detected in allocator %s at offset %d and address %d!\n", GetDebugName().c_str(),
                            newOffset, address);
        }

        if (IsAllocationTrackingEnabled)
        {
            AddDeallocation();
        }

        SetCurrentOffset(newOffset);
    }

    UIntPtr GetAddressFromPtr(const void* ptr) const
    {
        if constexpr (IsNullCheckEnabled)
        {
            MEMARENA_ASSERT(ptr, "Error: Cannot deallocate nullptr in allocator %s!\n", GetDebugName().c_str());
        }

        const UIntPtr address = std::bit_cast<UIntPtr>(ptr);

        if constexpr (IsOwnershipCheckEnabled)
        {
            MEMARENA_ASSERT(OwnsAddress(address), "Error: The allocator %s does not own the pointer %d!\n", GetDebugName().c_str(),
                            address);
        }

        return address;
    }

    template <Size headerSize>
    static consteval Size GetTotalHeaderSize()
    {
        if constexpr (IsBoundsCheckEnabled)
        {
            return headerSize + sizeof(BoundGuardFront);
        }
        else
        {
            return headerSize;
        }
    }

    void SetCurrentOffset(const Offset offset)
    {
        m_CurrentOffset = offset;

        if (IsUsageTrackingEnabled)
        {
            SetUsedSize(offset);
        }
    }

    [[nodiscard]] bool OwnsAddress(UIntPtr address) const { return address >= m_StartAddress && address <= m_EndAddress; }

    ThreadPolicy m_MultithreadedPolicy;

    // Dont change member variable declaration order in this block!
    void*   m_StartPtr = nullptr;
    UIntPtr m_StartAddress;
    UIntPtr m_EndAddress;
    // -------------------

    Offset m_CurrentOffset = 0;
};

// template <PoolAllocatorPolicy policy>
// function(PoolAllocator<>)->function(PoolAllocator<policy>);
} // namespace Memarena