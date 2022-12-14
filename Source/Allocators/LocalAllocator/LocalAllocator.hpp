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

    NO_DISCARD void* AllocateBase(const Size size) final { return m_Memory[0]; }
    void             DeallocateBase(void* ptr) final {}

  private:
    std::array<char, TotalSize> m_Memory;
};
} // namespace Memarena