#pragma once

#include <bit>
#include <vector>

#include "Source/Allocator.hpp"
#include "Source/AllocatorData.hpp"
#include "Source/AllocatorUtils.hpp"
#include "Source/Assert.hpp"
#include "Source/Macros.hpp"
#include "Source/Policies/MultithreadedPolicy.hpp"
#include "Source/Policies/Policies.hpp"
#include "Source/Utility/Alignment/Alignment.hpp"

namespace Memarena
{

/**
 * @brief A custom memory allocator that cannot deallocate individual allocations. To free allocations, you must
 *       free the entire arena by calling `Release`.
 *
 * @tparam policy
 */
template <LinearAllocatorPolicy policy = LinearAllocatorPolicy::Default>
class LinearAllocator : public Allocator
{
  private:
    static constexpr bool IsSizeCheckEnabled          = PolicyContains(policy, LinearAllocatorPolicy::SizeCheck);
    static constexpr bool IsGrowable                  = PolicyContains(policy, LinearAllocatorPolicy::Growable);
    static constexpr bool IsUsageTrackingEnabled      = PolicyContains(policy, LinearAllocatorPolicy::UsageTracking);
    static constexpr bool IsAllocationTrackingEnabled = PolicyContains(policy, LinearAllocatorPolicy::AllocationTracking);
    static constexpr bool IsMultithreaded             = PolicyContains(policy, LinearAllocatorPolicy::Multithreaded);

    using ThreadPolicy = MultithreadedPolicy<IsMultithreaded, IsGrowable>;

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

    explicit LinearAllocator(Size blockSize, const std::string& debugName = "LinearAllocator")
        : Allocator(blockSize, debugName), m_BlockSize(blockSize)
    {
        AllocateBlock();
    }

    ~LinearAllocator()
    {
        for (auto& blockPtr : m_BlockPtrs)
        {
            free(blockPtr);
        }
    };

    template <typename Object, typename... Args>
    NO_DISCARD Object* NewRaw(Args&&... argList)
    {
        void* voidPtr = Allocate<Object>();
        return new (voidPtr) Object(std::forward<Args>(argList)...);
    }

    template <typename Object, typename... Args>
    NO_DISCARD Object* NewArrayRaw(const Size objectCount, Args&&... argList)
    {
        void* voidPtr = AllocateArray<Object>(objectCount);
        return Internal::ConstructArray<Object>(voidPtr, objectCount, std::forward<Args>(argList)...);
    }

    NO_DISCARD void* Allocate(const Size size, const Alignment& alignment, const std::string& category = "",
                              const SourceLocation& sourceLocation = SourceLocation::current())
    {
        UIntPtr alignedAddress = 0;

        {
            // Scope to release the lock after the allocation
            LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

            const UIntPtr baseAddress = m_CurrentStartAddress + m_CurrentOffset;
            alignedAddress            = CalculateAlignedAddress(baseAddress, alignment);
            const Padding padding     = alignedAddress - baseAddress;

            Size totalSizeAfterAllocation = m_CurrentOffset + padding + size;
            SetCurrentOffset(totalSizeAfterAllocation);

            if constexpr (IsGrowable)
            {
                // TODO(Ahsan): Check if allocation will be more than max possible size
                if (totalSizeAfterAllocation > m_BlockSize)
                {
                    AllocateBlock();
                    IncreaseTotalSize(m_BlockSize);
                    guard.unlock();
                    return Allocate(size, alignment, category, sourceLocation);
                }
            }
            else if constexpr (IsSizeCheckEnabled)
            {
                MEMARENA_ASSERT(totalSizeAfterAllocation <= m_BlockSize, "Error: The allocator '%s' is out of memory!\n",
                                GetDebugName().c_str());
            }
        }

        if (IsAllocationTrackingEnabled)
        {
            AddAllocation(size, category, sourceLocation);
        }

        return std::bit_cast<void*>(alignedAddress);
    }

    template <typename Object>
    NO_DISCARD void* Allocate(const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return Allocate(sizeof(Object), alignof(Object), category, sourceLocation);
    }

    NO_DISCARD void* AllocateArray(const Size objectCount, const Size objectSize, const Alignment& alignment,
                                   const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        const Size allocationSize = objectCount * objectSize;
        return Allocate(allocationSize, alignment, category, sourceLocation);
    }

    template <typename Object>
    NO_DISCARD void* AllocateArray(const Size objectCount, const std::string& category = "",
                                   const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return AllocateArray(objectCount, sizeof(Object), alignof(Object), category, sourceLocation);
    }

    /**
     * @brief Releases the allocator to its initial state. Since LinearAllocators dont support de-allocating separate allocation, this is
     * how you clean the memory
     *
     */
    inline void Release()
    {
        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);
        DeallocateBlocks();
    };

  private:
    void SetCurrentOffset(const Offset offset)
    {
        m_CurrentOffset = offset;

        if (IsUsageTrackingEnabled)
        {
            SetUsedSize((m_BlockPtrs.size() - 1) * m_BlockSize + offset);
        }
    }

    inline void AllocateBlock()
    {
        void* newBlockPtr = malloc(m_BlockSize);
        m_BlockPtrs.push_back(newBlockPtr);
        m_CurrentStartAddress = std::bit_cast<UIntPtr>(m_BlockPtrs.back());
        m_CurrentOffset       = 0;

        if (IsUsageTrackingEnabled)
        {
            SetUsedSize((m_BlockPtrs.size() - 1) * m_BlockSize);
        }
    }

    // Deallocates all but the first block
    inline void DeallocateBlocks()
    {
        while (m_BlockPtrs.size() > 1)
        {
            FreeLastBlock();
        }

        m_CurrentStartAddress = std::bit_cast<UIntPtr>(m_BlockPtrs.back());
        SetCurrentOffset(0);
    }

    inline void FreeLastBlock()
    {
        free(m_BlockPtrs.back());
        m_BlockPtrs.pop_back();
    }

    ThreadPolicy m_MultithreadedPolicy;

    // Dont change member variable declaration order in this block!
    std::vector<void*> m_BlockPtrs;
    UIntPtr            m_CurrentStartAddress = 0;
    // ---------------------------------------

    Size   m_BlockSize;
    Offset m_CurrentOffset = 0;
};
} // namespace Memarena