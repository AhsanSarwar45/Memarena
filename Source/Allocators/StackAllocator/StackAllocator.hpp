#pragma once

#include <experimental/source_location>
#include <utility>

#include "Source/Allocator.hpp"
#include "Source/AllocatorData.hpp"
#include "Source/AllocatorSettings.hpp"
#include "Source/AllocatorUtils.hpp"
#include "Source/Assert.hpp"
#include "Source/Macros.hpp"
#include "Source/Policies/BoundsCheckPolicy.hpp"
#include "Source/Policies/MultithreadedPolicy.hpp"
#include "Source/Policies/Policies.hpp"
#include "Source/Traits.hpp"
#include "Source/Utility/Alignment/Alignment.hpp"

namespace Memarena
{
using StackAllocatorSettings = AllocatorSettings<StackAllocatorPolicy>;
constexpr StackAllocatorSettings stackAllocatorDefaultSettings{};

namespace Internal
{

struct StackHeaderLite
{
    Offset startOffset;

    StackHeaderLite(Offset _startOffset, Offset /*_endOffset*/) : startOffset(_startOffset) {}
};

struct StackHeader
{
    Offset startOffset;
    Offset endOffset;

    StackHeader(Offset _startOffset, Offset _endOffset) : startOffset(_startOffset), endOffset(_endOffset) {}
};

struct StackArrayHeader
{
    Offset startOffset;
    Offset count;

    StackArrayHeader(Offset _startOffset, Offset _count) : startOffset(_startOffset), count(_count) {}
};
} // namespace Internal

template <typename T>
class StackPtr : public Ptr<T>
{
    // Allow only StackAllocator to create a StackPtr by making constructors private
    template <StackAllocatorSettings Settings>
    friend class StackAllocator;

  public:
    [[nodiscard]] inline const Internal::StackHeader& GetHeader() const { return m_Header; }

  private:
    inline StackPtr(T* ptr, const Internal::StackHeader& header) : Ptr<T>(ptr), m_Header(header) {}
    inline StackPtr(T* ptr, Offset startOffset, Offset endOffset) : Ptr<T>(ptr), m_Header(startOffset, endOffset) {}
    Internal::StackHeader m_Header;
};

template <typename T>
class StackArrayPtr : public ArrayPtr<T>
{
    // Allow only StackAllocator to create a StackArrayPtr by making constructors private
    template <StackAllocatorSettings Settings>
    friend class StackAllocator;

  public:
    [[nodiscard]] inline Size                              GetCount() const { return m_Header.count; }
    [[nodiscard]] inline const Internal::StackArrayHeader& GetHeader() const { return m_Header; }

  private:
    StackArrayPtr(T* ptr, const Internal::StackArrayHeader& header) : ArrayPtr<T>(ptr, header.count), m_Header(header) {}
    StackArrayPtr(T* ptr, Offset startOffset, Offset count) : ArrayPtr<T>(ptr, count), m_Header(startOffset, count) {}

    Internal::StackArrayHeader m_Header;
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
 *
 * @tparam policy The `StackAllocatorPolicy`mjn object to define the behaviour of this allocator
 */
template <StackAllocatorSettings Settings = stackAllocatorDefaultSettings>
class StackAllocator : public Allocator
{
  private:
    static constexpr auto Policy = Settings.policy;

    static constexpr bool StackCheckIsEnabled         = PolicyContains(Policy, StackAllocatorPolicy::StackCheck);
    static constexpr bool BoundsCheckIsEnabled        = PolicyContains(Policy, StackAllocatorPolicy::BoundsCheck);
    static constexpr bool IsNullDeallocCheckEnabled   = PolicyContains(Policy, StackAllocatorPolicy::NullDeallocCheck);
    static constexpr bool SizeCheckIsEnabled          = PolicyContains(Policy, StackAllocatorPolicy::SizeCheck);
    static constexpr bool OwnershipIsCheckEnabled     = PolicyContains(Policy, StackAllocatorPolicy::OwnershipCheck);
    static constexpr bool UsageTrackingIsEnabled      = PolicyContains(Policy, StackAllocatorPolicy::SizeTracking);
    static constexpr bool IsMultithreaded             = PolicyContains(Policy, StackAllocatorPolicy::Multithreaded);
    static constexpr bool AllocationTrackingIsEnabled = PolicyContains(Policy, StackAllocatorPolicy::AllocationTracking);
    static constexpr bool IsResizable                 = PolicyContains(Policy, StackAllocatorPolicy::Resizable);

    using InplaceHeader      = typename std::conditional<StackCheckIsEnabled, Internal::StackHeader, Internal::StackHeaderLite>::type;
    using Header             = Internal::StackHeader;
    using InplaceArrayHeader = Internal::StackArrayHeader;
    using ArrayHeader        = Internal::StackArrayHeader;

    using ThreadPolicy = MultithreadedPolicy<IsMultithreaded>;

    template <typename SyncPrimitive>
    using LockGuard = typename ThreadPolicy::template LockGuard<SyncPrimitive>;
    using Mutex     = typename ThreadPolicy::Mutex;

    static constexpr Size BackGuardSize = BoundsCheckIsEnabled ? sizeof(BoundGuardBack) : 0;

  public:
    // Prohibit default construction, moving and assignment
    StackAllocator()                      = delete;
    StackAllocator(StackAllocator&)       = delete;
    StackAllocator(const StackAllocator&) = delete;
    StackAllocator(StackAllocator&&)      = delete;
    StackAllocator& operator=(const StackAllocator&) = delete;
    StackAllocator& operator=(StackAllocator&&) = delete;

    explicit StackAllocator(const Size totalSize, const std::string& debugName = "StackAllocator",
                            std::shared_ptr<Allocator> baseAllocator = Allocator::GetDefaultAllocator())
        : Allocator(totalSize, debugName), m_StartPtr(baseAllocator->AllocateBase(totalSize)),
          m_StartAddress(std::bit_cast<UIntPtr>(m_StartPtr)), m_EndAddress(m_StartAddress + totalSize),
          m_BaseAllocator(std::move(baseAllocator))
    {
    }

    ~StackAllocator() { m_BaseAllocator->DeallocateBase(m_StartPtr); };

    friend bool operator==(const StackAllocator& s1, const StackAllocator& s2) { return s1.m_StartAddress == s2.m_StartAddress; }

    template <Allocatable Object, typename... Args>
    NO_DISCARD StackPtr<Object> New(Args&&... argList)
    {
        auto [voidPtr, startOffset, endOffset] = AllocateInternal(sizeof(Object), alignof(Object));
        Object* objectPtr                      = new (voidPtr) Object(std::forward<Args>(argList)...);
        return StackPtr<Object>(objectPtr, startOffset, endOffset);
    }

    template <Allocatable Object, typename... Args>
    NO_DISCARD Object* NewRaw(Args&&... argList)
    {
        void*   voidPtr   = Allocate<Object>();
        Object* objectPtr = new (voidPtr) Object(std::forward<Args>(argList)...);
        return objectPtr;
    }

    template <Allocatable Object>
    void Delete(StackPtr<Object> ptr)
    {
        Deallocate(StackPtr<void>(ptr.GetPtr(), ptr.m_Header));
        ptr->~Object();
    }

    template <Allocatable Object>
    void Delete(Object* ptr)
    {
        Deallocate(ptr);
        ptr->~Object();
    }

    template <Allocatable Object, typename... Args>
    NO_DISCARD StackArrayPtr<Object> NewArray(const Size objectCount, Args&&... argList)
    {
        auto [voidPtr, startOffset, endOffset] = AllocateInternal(objectCount * sizeof(Object), alignof(Object));
        return StackArrayPtr<Object>(Internal::ConstructArray<Object>(voidPtr, objectCount, std::forward<Args>(argList)...), startOffset,
                                     objectCount);
    }

    template <Allocatable Object, typename... Args>
    NO_DISCARD Object* NewArrayRaw(const Size objectCount, Args&&... argList)
    {
        void* voidPtr = AllocateArray<Object>(objectCount);
        return Internal::ConstructArray<Object>(voidPtr, objectCount, std::forward<Args>(argList)...);
    }

    template <Allocatable Object>
    void DeleteArray(Object* ptr)
    {
        const Size objectCount = DeallocateArray(ptr, sizeof(Object));
        std::destroy_n(ptr, objectCount);
    }

    template <Allocatable Object>
    void DeleteArray(StackArrayPtr<Object> ptr)
    {
        const Size objectCount = DeallocateArray(StackArrayPtr<void>(ptr.GetPtr(), ptr.m_Header), sizeof(Object));
        std::destroy_n(ptr.GetPtr(), objectCount);
    }

    NO_DISCARD void* Allocate(const Size size, const Alignment& alignment = defaultAlignment, const std::string& category = "",
                              const SourceLocation& sourceLocation = SourceLocation::current())
    {
        auto [voidPtr, startOffset, endOffset] = AllocateInternal<sizeof(InplaceHeader)>(size, alignment, category, sourceLocation);
        Internal::AllocateHeader<InplaceHeader>(voidPtr, startOffset, endOffset);
        return voidPtr;
    }

    template <typename Object>
    NO_DISCARD void* Allocate(const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return Allocate(sizeof(Object), alignof(Object), category, sourceLocation);
    }

    void Deallocate(void* ptr)
    {
        const UIntPtr currentAddress = GetAddressFromPtr(ptr);
        auto [header, headerAddress] = Internal::GetHeaderFromAddress<InplaceHeader>(currentAddress);
        DeallocateInternal(currentAddress, headerAddress, header);
    }

    void Deallocate(const StackPtr<void>& ptr)
    {
        const void*   voidPtr        = ptr.GetPtr();
        const UIntPtr currentAddress = GetAddressFromPtr(voidPtr);
        DeallocateInternal(currentAddress, currentAddress, ptr.GetHeader());
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
        return AllocateArray(objectCount, sizeof(Object), alignof(Object), category, sourceLocation);
    }

    Size DeallocateArray(void* ptr, const Size objectSize)
    {
        const UIntPtr currentAddress = GetAddressFromPtr(ptr);
        auto [header, headerAddress] = Internal::GetHeaderFromAddress<InplaceArrayHeader>(currentAddress);
        DeallocateInternal(currentAddress, headerAddress,
                           Header(header.startOffset,
                                  Internal::GetArrayEndOffset(currentAddress, m_StartAddress, header.count, objectSize, BackGuardSize)));
        return header.count;
    }

    Size DeallocateArray(const StackArrayPtr<void>& ptr, const Size objectSize)
    {
        const void*                      voidPtr        = ptr.GetPtr();
        const UIntPtr                    currentAddress = GetAddressFromPtr(voidPtr);
        const Internal::StackArrayHeader header         = ptr.GetHeader();
        DeallocateInternal(currentAddress, currentAddress,
                           Header(header.startOffset,
                                  Internal::GetArrayEndOffset(currentAddress, m_StartAddress, header.count, objectSize, BackGuardSize)));
        return header.count;
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

    [[nodiscard]] bool OwnsAddress(UIntPtr address) const { return address >= m_StartAddress && address <= m_EndAddress; }

  private:
    template <Size HeaderSize = 0>
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

        if constexpr (SizeCheckIsEnabled)
        {
            MEMARENA_ASSERT(totalSizeAfterAllocation <= GetTotalSize(), "Error: The allocator '%s' is out of memory!\n",
                            GetDebugName().c_str());
        }

        if constexpr (BoundsCheckIsEnabled)
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

        if constexpr (AllocationTrackingIsEnabled)
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

        if constexpr (StackCheckIsEnabled)
        {
            MEMARENA_ASSERT(header.endOffset == m_CurrentOffset,
                            "Error: Attempt to deallocate in wrong order in the stack allocator '%s'!\n", GetDebugName().c_str());
        }

        if constexpr (BoundsCheckIsEnabled)
        {
            const UIntPtr          frontGuardAddress = addressMarker - sizeof(BoundGuardFront);
            const BoundGuardFront* frontGuard        = std::bit_cast<BoundGuardFront*>(frontGuardAddress);

            const UIntPtr         backGuardAddress = address + frontGuard->allocationSize;
            const BoundGuardBack* backGuard        = std::bit_cast<BoundGuardBack*>(backGuardAddress);

            MEMARENA_ASSERT(frontGuard->offset == newOffset && backGuard->offset == newOffset,
                            "Error: Memory stomping detected in allocator '%s' at offset %d and address %d!\n", GetDebugName().c_str(),
                            newOffset, address);
        }

        if constexpr (AllocationTrackingIsEnabled)
        {
            AddDeallocation();
        }

        SetCurrentOffset(newOffset);
    }

    UIntPtr GetAddressFromPtr(const void* ptr) const
    {
        if constexpr (IsNullDeallocCheckEnabled)
        {
            MEMARENA_ASSERT(ptr, "Error: Cannot deallocate nullptr in allocator '%s'!\n", GetDebugName().c_str());
        }

        const UIntPtr address = std::bit_cast<UIntPtr>(ptr);

        if constexpr (OwnershipIsCheckEnabled)
        {
            MEMARENA_ASSERT(OwnsAddress(address), "Error: The allocator '%s' does not own the pointer %d!\n", GetDebugName().c_str(),
                            address);
        }

        return address;
    }

    template <Size headerSize>
    static consteval Size GetTotalHeaderSize()
    {
        if constexpr (BoundsCheckIsEnabled)
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

        if constexpr (UsageTrackingIsEnabled)
        {
            SetUsedSize(m_CurrentOffset);
        }
    }

    ThreadPolicy m_MultithreadedPolicy;

    // Dont change member variable declaration order in this block!
    void*   m_StartPtr;
    UIntPtr m_StartAddress;
    UIntPtr m_EndAddress;
    // -------------------

    Offset m_CurrentOffset = 0;

    std::shared_ptr<Allocator> m_BaseAllocator;
};

// template <StackAllocatorPolicy policy>
// function(StackAllocator<>)->function(StackAllocator<policy>);
} // namespace Memarena