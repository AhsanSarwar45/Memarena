#pragma once

#include <bit>

#include "Source/Allocator.hpp"
#include "Source/AllocatorData.hpp"
#include "Source/AllocatorUtils.hpp"
#include "Source/Assert.hpp"
#include "Source/Policies/MultithreadedPolicy.hpp"
#include "Source/Policies/Policies.hpp"
#include "Source/Utility/Alignment.hpp"

#define NO_DISCARD_ALLOC_INFO "Not using the pointer returned will cause a soft memory leak!"

namespace Memarena
{

template <LinearAllocatorPolicy policy = LinearAllocatorPolicy::Default>
class LinearAllocator : public Allocator
{
  private:
    static constexpr bool IsSizeCheckEnabled          = PolicyContains(policy, StackAllocatorPolicy::SizeCheck);
    static constexpr bool IsUsageTrackingEnabled      = PolicyContains(policy, StackAllocatorPolicy::UsageTracking);
    static constexpr bool IsAllocationTrackingEnabled = PolicyContains(policy, StackAllocatorPolicy::AllocationTracking);

    using ThreadPolicy = MultithreadedPolicy<policy>;

    template <typename SyncPrimitive>
    using LockGuard = typename ThreadPolicy::template LockGuard<SyncPrimitive>;
    using Mutex     = typename ThreadPolicy::Mutex;

  public:
    // Prohibit default construction, moving and assignment
    LinearAllocator()                       = delete;
    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator(LinearAllocator&)       = delete;
    LinearAllocator(LinearAllocator&&)      = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;
    LinearAllocator& operator=(LinearAllocator&&) = delete;

    explicit LinearAllocator(const Size totalSize, const std::string& debugName = "LinearAllocator")
        : Allocator(totalSize, debugName), m_StartAddress(std::bit_cast<UIntPtr>(GetStartPtr()))
    {
    }

    ~LinearAllocator() = default;

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

    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* Allocate(const Size size, const Alignment& alignment, const std::string& category = "",
                                                        const SourceLocation& sourceLocation = SourceLocation::current())
    {
        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

        const UIntPtr baseAddress = m_StartAddress + m_CurrentOffset;

        UIntPtr alignedAddress = CalculateAlignedAddress(baseAddress, alignment);
        Padding padding        = alignedAddress - baseAddress;

        Size totalSizeAfterAllocation = m_CurrentOffset + padding + size;

        if constexpr (IsSizeCheckEnabled)
        {
            MEMARENA_ASSERT(totalSizeAfterAllocation <= GetTotalSize(), "Error: The allocator %s is out of memory!\n",
                            GetDebugName().c_str());
        }

        SetCurrentOffset(totalSizeAfterAllocation);

        if (IsAllocationTrackingEnabled)
        {
            AddAllocation(size, category, sourceLocation);
        }

        return std::bit_cast<void*>(alignedAddress);
    }

    template <typename Object>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* Allocate(const std::string&    category       = "",
                                                        const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return Allocate(sizeof(Object), AlignOf(alignof(Object)), category, sourceLocation);
    }

    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* AllocateArray(const Size objectCount, const Size objectSize, const Alignment& alignment,
                                                             const std::string&    category       = "",
                                                             const SourceLocation& sourceLocation = SourceLocation::current())
    {
        const Size allocationSize = objectCount * objectSize;
        return Allocate(allocationSize, alignment, category, sourceLocation);
    }

    template <typename Object>
    [[nodiscard(NO_DISCARD_ALLOC_INFO)]] void* AllocateArray(const Size objectCount, const std::string& category = "",
                                                             const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return AllocateArray(objectCount, sizeof(Object), AlignOf(alignof(Object)), category, sourceLocation);
    }

    /**
     * @brief Resets the allocator to its initial state. Since LinearAllocators dont support de-alllocating separate allocation, this is how
     * you clean the memory
     *
     */
    inline void Reset()
    {
        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);
        SetCurrentOffset(0);
    };

  private:
    void SetCurrentOffset(const Offset offset)
    {
        m_CurrentOffset = offset;

        if (IsUsageTrackingEnabled)
        {
            SetUsedSize(offset);
        }
    }

    ThreadPolicy m_MultithreadedPolicy;

    UIntPtr m_StartAddress;
    Offset  m_CurrentOffset = 0;
};
} // namespace Memarena