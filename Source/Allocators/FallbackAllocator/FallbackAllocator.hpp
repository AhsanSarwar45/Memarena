#pragma once

#include <bit>
#include <type_traits>
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

template <typename T>
concept PrimaryAllocatable = requires(T a)
{
    {
        a.Allocate(0)
        } -> std::same_as<Ptr<void>>;

    a.Deallocate(Ptr<void>(nullptr));
    {
        a.OwnsAddress(0)
        } -> std::same_as<bool>;
}
|| requires(T a)
{
    {
        a.Allocate(0)
        } -> std::same_as<void*>;

    a.Deallocate(nullptr);
    {
        a.OwnsAddress(0)
        } -> std::same_as<bool>;
};

template <typename T>
concept FallbackAllocatable = requires(T a)
{
    {
        a.Allocate(0)
        } -> std::same_as<Ptr<void>>;
    {
        a.Deallocate(nullptr)
        } -> std::same_as<Ptr<void>>;
};

// template <PrimaryAllocatable PrimaryAllocator>
// struct Set
// {
// };

// Set<StackAllocator> set;

using FallbackAllocatorSettings = AllocatorSettings<FallbackAllocatorPolicy>;
constexpr FallbackAllocatorSettings fallbackAllocatorDefaultSettings{};

/**
 * @brief A custom memory allocator that cannot deallocate individual allocations. To free allocations, you must
 *       free the entire arena by calling `Release`.
 *
 * @tparam policy
 */
template <PrimaryAllocatable PrimaryAllocatorType, FallbackAllocatable FallbackAllocatorType,
          FallbackAllocatorSettings Settings = fallbackAllocatorDefaultSettings>
class FallbackAllocator : public Allocator
{
  private:
    static constexpr auto Policy = Settings.policy;

    static constexpr bool UsageTrackingIsEnabled      = PolicyContains(Policy, FallbackAllocatorPolicy::SizeTracking);
    static constexpr bool AllocationTrackingIsEnabled = PolicyContains(Policy, FallbackAllocatorPolicy::AllocationTracking);
    static constexpr bool IsMultithreaded             = PolicyContains(Policy, FallbackAllocatorPolicy::Multithreaded);

    // using ThreadPolicy = MultithreadedPolicy<IsMultithreaded, IsGrowable>;

    // template <typename SyncPrimitive>
    // using LockGuard = typename ThreadPolicy::template LockGuard<SyncPrimitive>;
    // using Mutex     = typename ThreadPolicy::Mutex;

  public:
    // Prohibit default construction, moving and assignment
    FallbackAllocator()                         = delete;
    FallbackAllocator(const FallbackAllocator&) = delete;
    FallbackAllocator(FallbackAllocator&)       = delete;
    FallbackAllocator(FallbackAllocator&&)      = delete;
    FallbackAllocator& operator=(const FallbackAllocator&) = delete;
    FallbackAllocator& operator=(FallbackAllocator&&) = delete;

    explicit FallbackAllocator(PrimaryAllocatorType primaryAllocator, FallbackAllocatorType fallbackAllocator,
                               const std::string&         debugName     = "FallbackAllocator",
                               std::shared_ptr<Allocator> baseAllocator = Allocator::GetDefaultAllocator())
        : Allocator(0, debugName), m_PrimaryAllocator(std::move(primaryAllocator)), m_FallbackAllocator(std::move(fallbackAllocator))
    {
        AllocateBlock();
    }

    ~FallbackAllocator(){

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
        const void* primaryPtr = m_PrimaryAllocator.Allocate(size, alignment, category, sourceLocation);

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
     * @brief Releases the allocator to its initial state. Since FallbackAllocators dont support de-allocating separate allocation, this
     * is how you clean the memory
     *
     */
    inline void Release()
    {
        LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);
        DeallocateBlocks();
    };

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

        Internal::BaseAllocatorPtr<void> newBlockPtr = m_BaseAllocator->AllocateBase(m_BlockSize);
        m_BlockPtrs.push_back(newBlockPtr);
        m_CurrentStartAddress = std::bit_cast<UIntPtr>(m_BlockPtrs.back().GetPtr());
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

        m_CurrentStartAddress = std::bit_cast<UIntPtr>(m_BlockPtrs[0].GetPtr());

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

    // ThreadPolicy m_MultithreadedPolicy;

    PrimaryAllocatorType  m_PrimaryAllocator;
    FallbackAllocatorType m_FallbackAllocator;
};
} // namespace Memarena