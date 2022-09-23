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
#include "Source/Traits.hpp"
#include "Source/Utility/Alignment/Alignment.hpp"

namespace Memarena
{

// template <typename T>
// concept Allocatable = requires(T a)
// {
//     {
//         std::hash<T>{}(a)
//         } -> std::convertible_to<std::size_t>;
// };

template <typename T>
using MallocPtr = Internal::BaseAllocatorPtr<T>;
template <typename T>
using MallocArrayPtr = Internal::BaseAllocatorArrayPtr<T>;

/**
 * @brief A custom memory allocator that cannot deallocate individual allocations. To free allocations, you must
 *       free the entire arena by calling `Release`.
 *
 * @tparam policy
 */
template <MallocatorPolicy policy = MallocatorPolicy::Default>
class Mallocator : public Allocator
{
  private:
    static constexpr bool IsDoubleFreePreventionEnabled = PolicyContains(policy, MallocatorPolicy::DoubleFreePrevention);
    static constexpr bool IsNullDeallocCheckEnabled =
        PolicyContains(policy, MallocatorPolicy::NullDeallocCheck) || IsDoubleFreePreventionEnabled;
    static constexpr bool IsNullAllocCheckEnabled     = PolicyContains(policy, MallocatorPolicy::NullAllocCheck);
    static constexpr bool IsAllocationTrackingEnabled = PolicyContains(policy, MallocatorPolicy::AllocationTracking);
    static constexpr bool IsSizeTrackingEnabled       = PolicyContains(policy, MallocatorPolicy::SizeTracking);
    static constexpr bool NeedsMultithreading         = IsAllocationTrackingEnabled || IsSizeTrackingEnabled;
    static constexpr bool IsMultithreaded             = PolicyContains(policy, MallocatorPolicy::Multithreaded) && NeedsMultithreading;

    using ThreadPolicy = MultithreadedPolicy<IsMultithreaded>;

    template <typename SyncPrimitive>
    using LockGuard = typename ThreadPolicy::template LockGuard<SyncPrimitive>;
    using Mutex     = typename ThreadPolicy::Mutex;

  public:
    // Prohibit default construction, moving and assignment
    // Mallocator()                  = delete;
    Mallocator(const Mallocator&) = delete;
    Mallocator(Mallocator&)       = delete;
    Mallocator(Mallocator&&)      = delete;
    Mallocator& operator=(const Mallocator&) = delete;
    Mallocator& operator=(Mallocator&&) = delete;

    Mallocator() : Allocator(0, "Mallocator", true) {}
    explicit Mallocator(const std::string& debugName) : Allocator(0, debugName, true) {}

    ~Mallocator() = default;

    template <Allocatable Object, typename... Args>
    NO_DISCARD MallocPtr<Object> New(Args&&... argList)
    {
        MallocPtr<void> voidPtr   = Allocate<Object>();
        Object*         objectPtr = new (voidPtr.GetPtr()) Object(std::forward<Args>(argList)...);
        return MallocPtr<Object>(objectPtr, sizeof(Object));
    }

    template <Allocatable Object>
    void Delete(MallocPtr<Object>& ptr)
    {
        DeallocateInternal(ptr, ptr.GetSize());
        ptr->~Object();
    }

    template <Allocatable Object, typename... Args>
    NO_DISCARD MallocArrayPtr<Object> NewArray(const Size objectCount, Args&&... argList)
    {
        MallocPtr<void> voidPtr   = AllocateArray<Object>(objectCount);
        Object*         objectPtr = Internal::ConstructArray<Object>(voidPtr.GetPtr(), objectCount, std::forward<Args>(argList)...);
        return MallocArrayPtr<Object>(objectPtr, objectCount * sizeof(Object), objectCount);
    }

    template <Allocatable Object>
    void DeleteArray(MallocArrayPtr<Object>& ptr)
    {
        DeallocateInternal(ptr, ptr.GetSize());
        std::destroy_n(ptr.GetPtr(), ptr.GetCount());
    }

    NO_DISCARD MallocPtr<void> Allocate(const Size size, const std::string& category = "",
                                        const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return MallocPtr<void>(AllocateInternal(size, category, sourceLocation), size);
    }

    template <typename Object>
    NO_DISCARD MallocPtr<void> Allocate(const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return Allocate(sizeof(Object), category, sourceLocation);
    }

    NO_DISCARD MallocPtr<void> AllocateArray(const Size objectCount, const Size objectSize, const std::string& category = "",
                                             const SourceLocation& sourceLocation = SourceLocation::current())
    {
        const Size allocationSize = objectCount * objectSize;
        return MallocPtr<void>(AllocateInternal(allocationSize, category, sourceLocation), allocationSize);
    }

    template <typename Object>
    NO_DISCARD MallocPtr<void> AllocateArray(const Size objectCount, const std::string& category = "",
                                             const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return AllocateArray(objectCount, sizeof(Object), category, sourceLocation);
    }

    void Deallocate(MallocPtr<void>& ptr) { DeallocateInternal(ptr, ptr.GetSize()); }
    void Deallocate(void* ptr, Size size) { DeallocateInternal(ptr, size); }
    void Deallocate(MallocArrayPtr<void>& ptr) { DeallocateInternal(ptr, ptr.GetSize()); }

    NO_DISCARD Internal::BaseAllocatorPtr<void> AllocateBase(const Size size) final { return Allocate(size); }
    void                                        DeallocateBase(Internal::BaseAllocatorPtr<void> ptr) final { Deallocate(ptr); }

  private:
    NO_DISCARD void* AllocateInternal(const Size size, const std::string& category = "",
                                      const SourceLocation& sourceLocation = SourceLocation::current())
    {

        void* ptr = malloc(size);

        if constexpr (IsNullAllocCheckEnabled)
        {
            MEMARENA_ASSERT(ptr != nullptr, "Error: The allocator '%s' couldn't allocate any memory!\n", GetDebugName().c_str());
        }
        {
            LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

            if constexpr (IsAllocationTrackingEnabled)
            {
                AddAllocation(size, category, sourceLocation);
            }
            if constexpr (IsSizeTrackingEnabled)
            {
                IncreaseTotalSize(size);
                IncreaseUsedSize(size);
            }
        }

        return malloc(size);
    }

    template <typename Object>
    void DeallocateInternal(Ptr<Object>& ptr, Size size)
    {
        DeallocateInternal(static_cast<void*>(ptr.GetPtr()), size);

        if constexpr (IsDoubleFreePreventionEnabled)
        {
            ptr.Reset();
        }
    }

    void DeallocateInternal(void* ptr, Size size)
    {
        if constexpr (IsNullDeallocCheckEnabled)
        {
            MEMARENA_ASSERT(ptr, "Error: Cannot deallocate nullptr in allocator '%s'!\n", GetDebugName().c_str());
        }

        free(static_cast<void*>(ptr));

        {
            LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

            if constexpr (IsAllocationTrackingEnabled)
            {
                AddDeallocation();
            }
            if constexpr (IsSizeTrackingEnabled)
            {

                DecreaseTotalSize(size);
                DecreaseUsedSize(size);
            }
        }
    }

    ThreadPolicy m_MultithreadedPolicy;
};
} // namespace Memarena