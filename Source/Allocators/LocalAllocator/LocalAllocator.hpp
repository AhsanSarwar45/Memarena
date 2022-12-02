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

template <typename T>
using LocalPtr = Internal::BaseAllocatorPtr<T>;
template <typename T>
using LocalArrayPtr = Internal::BaseAllocatorArrayPtr<T>;

template <Size TotalSize, LocalAllocatorPolicy policy = GetDefaultPolicy<LocalAllocatorPolicy>()>
class LocalAllocator : public Allocator
{
  private:
    static constexpr bool IsDoubleFreePreventionEnabled = PolicyContains(policy, LocalAllocatorPolicy::DoubleFreePrevention);
    static constexpr bool IsNullDeallocCheckEnabled =
        PolicyContains(policy, LocalAllocatorPolicy::NullDeallocCheck) || IsDoubleFreePreventionEnabled;
    static constexpr bool IsNullAllocCheckEnabled     = PolicyContains(policy, LocalAllocatorPolicy::NullAllocCheck);
    static constexpr bool AllocationTrackingIsEnabled = PolicyContains(policy, LocalAllocatorPolicy::AllocationTracking);
    static constexpr bool IsSizeTrackingEnabled       = PolicyContains(policy, LocalAllocatorPolicy::SizeTracking);
    static constexpr bool NeedsMultithreading         = AllocationTrackingIsEnabled || IsSizeTrackingEnabled;
    static constexpr bool IsMultithreaded             = PolicyContains(policy, LocalAllocatorPolicy::Multithreaded) && NeedsMultithreading;

    using ThreadPolicy = MultithreadedPolicy<IsMultithreaded>;

    template <typename SyncPrimitive>
    using LockGuard = typename ThreadPolicy::template LockGuard<SyncPrimitive>;
    using Mutex     = typename ThreadPolicy::Mutex;

  public:
    // Prohibit default construction, moving and assignment
    // LocalAllocator()                  = delete;
    LocalAllocator(const LocalAllocator&) = delete;
    LocalAllocator(LocalAllocator&)       = delete;
    LocalAllocator(LocalAllocator&&)      = delete;
    LocalAllocator& operator=(const LocalAllocator&) = delete;
    LocalAllocator& operator=(LocalAllocator&&) = delete;

    LocalAllocator() : Allocator(0, "LocalAllocator", true) {}
    explicit LocalAllocator(const std::string& debugName) : Allocator(0, debugName, true) {}

    ~LocalAllocator() = default;

    template <Allocatable Object, typename... Args>
    NO_DISCARD LocalPtr<Object> New(Args&&... argList)
    {
        LocalPtr<void> voidPtr   = Allocate<Object>();
        Object*        objectPtr = new (voidPtr.GetPtr()) Object(std::forward<Args>(argList)...);
        return LocalPtr<Object>(objectPtr, sizeof(Object));
    }

    template <Allocatable Object>
    void Delete(LocalPtr<Object>& ptr)
    {
        DeallocateInternal(ptr, ptr.GetSize());
        ptr->~Object();
    }

    template <Allocatable Object, typename... Args>
    NO_DISCARD LocalArrayPtr<Object> NewArray(const Size objectCount, Args&&... argList)
    {
        LocalPtr<void> voidPtr   = AllocateArray<Object>(objectCount);
        Object*        objectPtr = Internal::ConstructArray<Object>(voidPtr.GetPtr(), objectCount, std::forward<Args>(argList)...);
        return LocalArrayPtr<Object>(objectPtr, objectCount * sizeof(Object), objectCount);
    }

    template <Allocatable Object>
    void DeleteArray(LocalArrayPtr<Object>& ptr)
    {
        DeallocateInternal(ptr, ptr.GetSize());
        std::destroy_n(ptr.GetPtr(), ptr.GetCount());
    }

    NO_DISCARD LocalPtr<void> Allocate(const Size size, const std::string& category = "",
                                       const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return LocalPtr<void>(AllocateInternal(size, category, sourceLocation), size);
    }

    template <typename Object>
    NO_DISCARD LocalPtr<void> Allocate(const std::string& category = "", const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return Allocate(sizeof(Object), category, sourceLocation);
    }

    NO_DISCARD LocalPtr<void> AllocateArray(const Size objectCount, const Size objectSize, const std::string& category = "",
                                            const SourceLocation& sourceLocation = SourceLocation::current())
    {
        const Size allocationSize = objectCount * objectSize;
        return LocalPtr<void>(AllocateInternal(allocationSize, category, sourceLocation), allocationSize);
    }

    template <typename Object>
    NO_DISCARD LocalPtr<void> AllocateArray(const Size objectCount, const std::string& category = "",
                                            const SourceLocation& sourceLocation = SourceLocation::current())
    {
        return AllocateArray(objectCount, sizeof(Object), category, sourceLocation);
    }

    void Deallocate(LocalPtr<void>& ptr) { DeallocateInternal(ptr, ptr.GetSize()); }
    void Deallocate(void* ptr, Size size) { DeallocateInternal(ptr, size); }
    void Deallocate(LocalArrayPtr<void>& ptr) { DeallocateInternal(ptr, ptr.GetSize()); }

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

            if constexpr (AllocationTrackingIsEnabled)
            {
                AddAllocation(size, category, sourceLocation);
            }
            if constexpr (IsSizeTrackingEnabled)
            {
                IncreaseTotalSize(size);
                IncreaseUsedSize(size);
            }
        }

        return ptr;
        ;
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

            if constexpr (AllocationTrackingIsEnabled)
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
    char         m_Memory[TotalSize];
};
} // namespace Memarena