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

template <typename T>
using MallocPtr = BaseAllocatorPtr<T>;
template <typename T>
using MallocArrayPtr = BaseAllocatorArrayPtr<T>;

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

    template <typename Object, typename... Args>
    NO_DISCARD MallocPtr<Object> New(Args&&... argList)
    {
        MallocPtr<void> voidPtr   = Allocate<Object>();
        Object*         objectPtr = new (voidPtr.GetPtr()) Object(std::forward<Args>(argList)...);
        return MallocPtr<Object>(objectPtr, sizeof(Object));
    }

    template <typename Object>
    void Delete(MallocPtr<Object> ptr)
    {
        Deallocate(MallocPtr<void>(ptr.GetPtr(), ptr.GetSize()));
        ptr->~Object();
    }

    template <typename Object, typename... Args>
    NO_DISCARD MallocArrayPtr<Object> NewArray(const Size objectCount, Args&&... argList)
    {
        MallocPtr<void> voidPtr   = AllocateArray<Object>(objectCount);
        Object*         objectPtr = Internal::ConstructArray<Object>(voidPtr.GetPtr(), objectCount, std::forward<Args>(argList)...);
        return MallocArrayPtr<Object>(objectPtr, objectCount * sizeof(Object), objectCount);
    }

    template <typename Object>
    void DeleteArray(MallocArrayPtr<Object> ptr)
    {
        Deallocate(MallocArrayPtr<void>(ptr.GetPtr(), ptr.GetSize(), ptr.GetCount()));
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

    void Deallocate(MallocPtr<void> ptr) { DeallocateInternal(ptr.GetPtr(), ptr.GetSize()); }
    void Deallocate(MallocArrayPtr<void> ptr) { DeallocateInternal(ptr.GetPtr(), ptr.GetSize()); }

    NO_DISCARD BaseAllocatorPtr<void> AllocateBase(const Size size) final { return Allocate(size); }
    void                              DeallocateBase(BaseAllocatorPtr<void> ptr) final { Deallocate(ptr); }

  private:
    NO_DISCARD void* AllocateInternal(const Size size, const std::string& category = "",
                                      const SourceLocation& sourceLocation = SourceLocation::current())
    {

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

    void DeallocateInternal(void* ptr, Size size)
    {

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
        free(ptr);
        // ptr.Reset();
        // TODO: implement reset
    }

    ThreadPolicy m_MultithreadedPolicy;
};
} // namespace Memarena