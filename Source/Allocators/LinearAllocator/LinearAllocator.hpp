#pragma once

#include <algorithm>
#include <bit>
#include <utility>
#include <vector>

#include "Source/Allocator.hpp"
#include "Source/AllocatorData.hpp"
#include "Source/AllocatorSettings.hpp"
#include "Source/AllocatorUtils.hpp"
#include "Source/Assert.hpp"
#include "Source/Macros.hpp"
#include "Source/Policies/MultithreadedPolicy.hpp"
#include "Source/Policies/Policies.hpp"
#include "Source/Traits.hpp"
#include "Source/Utility/Alignment/Alignment.hpp"

namespace Memarena
{

using LinearAllocatorSettings = AllocatorSettings<LinearAllocatorPolicy>;
constexpr LinearAllocatorSettings linearAllocatorDefaultSettings{};

/**
 * @brief A custom memory allocator that cannot deallocate individual allocations. To free allocations, you must
 *       free the entire arena by calling `Release`.
 *
 * @tparam policy
 */
template <LinearAllocatorSettings Settings = linearAllocatorDefaultSettings>
class LinearAllocator : public Allocator
{
  private:
    static constexpr auto Policy = Settings.policy;

    static constexpr bool SizeCheckIsEnabled          = PolicyContains(Policy, LinearAllocatorPolicy::SizeCheck);
    static constexpr bool IsGrowable                  = PolicyContains(Policy, LinearAllocatorPolicy::Growable);
    static constexpr bool UsageTrackingIsEnabled      = PolicyContains(Policy, LinearAllocatorPolicy::SizeTracking);
    static constexpr bool AllocationTrackingIsEnabled = PolicyContains(Policy, LinearAllocatorPolicy::AllocationTracking);
    static constexpr bool IsMultithreaded             = PolicyContains(Policy, LinearAllocatorPolicy::Multithreaded);

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

    explicit LinearAllocator(const Size blockSize, const std::string& debugName = "LinearAllocator",
                             std::shared_ptr<Allocator> baseAllocator = Allocator::GetDefaultAllocator())
        : Allocator(blockSize, debugName), m_BlockSize(blockSize), m_BaseAllocator(std::move(baseAllocator))
    {
        AllocateBlock();
    }

    ~LinearAllocator()
    {
        for (auto& blockPtr : m_BlockPtrs)
        {
            m_BaseAllocator->DeallocateBase(blockPtr);
        }
    };

    template <Allocatable Object, typename... Args>
    NO_DISCARD Object* NewRaw(Args&&... argList)
    {
        void* voidPtr = Allocate<Object>();
        RETURN_IF_NULLPTR(voidPtr);
        Object* ptr = static_cast<Object*>(voidPtr);

        // return new (voidPtr) Object(std::forward<Args>(argList)...);
        return std::construct_at(ptr, std::forward<Args>(argList)...);
    }

    template <Allocatable Object, typename... Args>
    NO_DISCARD Object* NewArrayRaw(const Size objectCount, Args&&... argList)
    {
        void* voidPtr = AllocateArray<Object>(objectCount);
        RETURN_IF_NULLPTR(voidPtr);
        return Internal::ConstructArray<Object>(voidPtr, objectCount, std::forward<Args>(argList)...);
    }

    NO_DISCARD void* Allocate(const Size size, const Alignment& alignment = defaultAlignment, const std::string& category = "",
                              const SourceLocation& sourceLocation = SourceLocation::current())
    {
        if constexpr (SizeCheckIsEnabled)
        {
            MEMARENA_ASSERT_RETURN(size <= m_BlockSize, nullptr,
                                   "Error: Allocation size (%u) must be <= to block size (%u) for allocator '%s'!\n", size, m_BlockSize,
                                   GetDebugName().c_str());
        }

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
                    guard.unlock();
                    return Allocate(size, alignment, category, sourceLocation);
                }
            }
            else
            {
                MEMARENA_ASSERT_RETURN(totalSizeAfterAllocation <= m_BlockSize, nullptr, "Error: The allocator '%s' is out of memory!\n",
                                       GetDebugName().c_str());
            }
        }

        if constexpr (AllocationTrackingIsEnabled)
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
     * @brief Releases the allocator to its initial state. Since LinearAllocators dont support de-allocating separate allocation, this
     * is how you clean the memory
     *
     */
    inline void Release()
    {
        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);
        DeallocateBlocks();
    };

    [[nodiscard]] bool Owns(UIntPtr address) const
    {
        return std::ranges::any_of(m_BlockPtrs, [&](void* blockPtr) {
            const UIntPtr startAddress = std::bit_cast<UIntPtr>(blockPtr);
            const UIntPtr endAddress   = startAddress + m_BlockSize;
            return address >= startAddress && address <= endAddress;
        });
    }
    [[nodiscard]] bool Owns(void* ptr) const { return Owns(std::bit_cast<UIntPtr>(ptr)); }
    template <typename Object>
    [[nodiscard]] bool Owns(Ptr<Object> ptr) const
    {
        return Owns(ptr.GetPtr());
    }

    // NO_DISCARD BaseAllocatorPtr<void> AllocateBase(const Size size) final { return Allocate(size); }

  private:
    void SetCurrentOffset(const Offset offset)
    {
        m_CurrentOffset = offset;

        if constexpr (UsageTrackingIsEnabled)
        {
            SetUsedSize((m_BlockPtrs.size() - 1) * m_BlockSize + offset);
        }
    }

    inline void AllocateBlock()
    {

        void* newBlockPtr = m_BaseAllocator->AllocateBase(m_BlockSize);
        m_BlockPtrs.push_back(newBlockPtr);
        m_CurrentStartAddress = std::bit_cast<UIntPtr>(m_BlockPtrs.back());
        m_CurrentOffset       = 0;

        if constexpr (UsageTrackingIsEnabled)
        {
            SetUsedSize((m_BlockPtrs.size() - 1) * m_BlockSize);
        }
        UpdateTotalSize();
    }

    // Deallocates all but the first block
    inline void DeallocateBlocks()
    {
        if constexpr (IsGrowable)
        {
            while (m_BlockPtrs.size() > 1)
            {
                FreeLastBlock();
            }
            UpdateTotalSize();
        }

        m_CurrentStartAddress = std::bit_cast<UIntPtr>(m_BlockPtrs[0]);

        SetCurrentOffset(0);
    }

    inline void FreeLastBlock()
    {
        m_BaseAllocator->DeallocateBase(m_BlockPtrs.back());
        m_BlockPtrs.pop_back();
    }

    inline void UpdateTotalSize()
    {
        if constexpr (UsageTrackingIsEnabled)
        {
            SetTotalSize(m_BlockPtrs.size() * m_BlockSize);
        }
    }

    ThreadPolicy m_MultithreadedPolicy;

    // Dont change member variable declaration order in this block!
    std::vector<void*> m_BlockPtrs;
    UIntPtr            m_CurrentStartAddress = 0;
    // ---------------------------------------

    Size   m_BlockSize;
    Offset m_CurrentOffset = 0;

    std::shared_ptr<Allocator> m_BaseAllocator;
};
} // namespace Memarena