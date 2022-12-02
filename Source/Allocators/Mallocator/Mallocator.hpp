#pragma once

#include <bit>
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
using MallocPtr = Internal::BaseAllocatorPtr<T>;
template <typename T>
using MallocArrayPtr = Internal::BaseAllocatorArrayPtr<T>;

using MallocatorSettings = AllocatorSettings<MallocatorPolicy>;
constexpr MallocatorSettings mallocatorDefaultSettings{};

template <MallocatorSettings Settings = mallocatorDefaultSettings>
class Mallocator : public Allocator
{
  private:
    static constexpr MallocatorPolicy Policy = Settings.policy;

    static constexpr bool DoubleFreePreventionIsEnabled = PolicyContains(Policy, MallocatorPolicy::DoubleFreePrevention);
    static constexpr bool NullDeallocCheckIsEnabled =
        PolicyContains(Policy, MallocatorPolicy::NullDeallocCheck) || DoubleFreePreventionIsEnabled;
    static constexpr bool NullAllocCheckIsEnabled     = PolicyContains(Policy, MallocatorPolicy::NullAllocCheck);
    static constexpr bool AllocationTrackingIsEnabled = PolicyContains(Policy, MallocatorPolicy::AllocationTracking);
    static constexpr bool SizeTrackingIsEnabled       = PolicyContains(Policy, MallocatorPolicy::SizeTracking);
    static constexpr bool NeedsMultithreading         = AllocationTrackingIsEnabled || SizeTrackingIsEnabled;
    static constexpr bool IsMultithreaded             = PolicyContains(Policy, MallocatorPolicy::Multithreaded) && NeedsMultithreading;

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
        MallocPtr<void> voidPtr = Allocate<Object>();
        RETURN_VAL_IF_NULLPTR(voidPtr.GetPtr(), MallocPtr<Object>(nullptr, 0));
        Object* objectPtr = new (voidPtr.GetPtr()) Object(std::forward<Args>(argList)...);
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
        MallocPtr<void> voidPtr = AllocateArray<Object>(objectCount);
        RETURN_VAL_IF_NULLPTR(voidPtr.GetPtr(), MallocArrayPtr<Object>(nullptr, 0, 0));
        Object* objectPtr = Internal::ConstructArray<Object>(voidPtr.GetPtr(), objectCount, std::forward<Args>(argList)...);
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

        if constexpr (NullAllocCheckIsEnabled)
        {
            MEMARENA_ASSERT_RETURN(ptr != nullptr, nullptr, "Error: The allocator '%s' couldn't allocate any memory!\n",
                                   GetDebugName().c_str());
        }
        {
            LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

            if constexpr (AllocationTrackingIsEnabled)
            {
                AddAllocation(size, category, sourceLocation);
            }
            if constexpr (SizeTrackingIsEnabled)
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

        if constexpr (DoubleFreePreventionIsEnabled)
        {
            ptr.Reset();
        }
    }

    void DeallocateInternal(void* ptr, Size size)
    {
        if constexpr (NullDeallocCheckIsEnabled)
        {
            MEMARENA_ASSERT_RETURN(ptr, void(), "Error: Cannot deallocate nullptr in allocator '%s'!\n", GetDebugName().c_str());
        }

        free(static_cast<void*>(ptr));

        {
            LockGuard<Mutex> guard(m_MultithreadedPolicy.m_Mutex);

            if constexpr (AllocationTrackingIsEnabled)
            {
                AddDeallocation();
            }
            if constexpr (SizeTrackingIsEnabled)
            {

                DecreaseTotalSize(size);
                DecreaseUsedSize(size);
            }
        }
    }

    ThreadPolicy m_MultithreadedPolicy;
};
} // namespace Memarena