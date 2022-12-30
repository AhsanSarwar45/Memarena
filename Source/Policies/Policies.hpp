#pragma once

#include <string>

#include "Source/Utility/Enums.hpp"

namespace Memarena
{

template <typename Enum>
struct IsPolicy
{
    static const bool value = false;
};

#define MARK_AS_POLICY(x)               \
    ENABLE_BITMASK_OPERATORS(x)         \
    template <>                         \
    struct IsPolicy<x>                  \
    {                                   \
        static const bool value = true; \
    };

#define BASE_ALLOCATOR_POLICIES                                                                                        \
    Empty = 0, AllocationTracking = Bit(27), /* Track the amount of allocations and deallocations of this allocator */ \
        SizeTracking  = Bit(28),             /* Track the amount of space used by this allocator */                    \
        Multithreaded = Bit(29)              /* Make allocations thread-safe. This will also make them blocking */

#define ALLOCATOR_POLICIES BASE_ALLOCATOR_POLICIES

template <typename Policy, typename Value>
constexpr bool PolicyContains(Policy policy, Value value)
{
    return static_cast<typename std::underlying_type<Policy>::type>(policy) &
           static_cast<typename std::underlying_type<Value>::type>(value);
}

template <typename Policy>
constexpr UInt32 PolicyToInt(Policy policy)
{
    return static_cast<UInt32>(policy);
}

enum class FallbackAllocatorPolicy : UInt32
{
    // BASE_ALLOCATOR_POLICIES,

    Default = 0,
    Release = 0,
    Debug   = 0,
};

MARK_AS_POLICY(FallbackAllocatorPolicy);

enum class StackAllocatorPolicy : UInt32
{
    ALLOCATOR_POLICIES,

    NullDeallocCheck     = Bit(0), // Check if the pointer is null when deallocating
    OwnershipCheck       = Bit(1), // Check if the pointer is owned/allocated by the allocator that is deallocating it
    BoundsCheck          = Bit(2), // Check if an allocation overwrites another allocation
    StackCheck           = Bit(3), // Check is deallocations are performed in LIFO order
    Resizable            = Bit(4), // Allow the allocator to grow when memory is exhausted
    DoubleFreePrevention = Bit(5), // Set the ptr to null on free to prevent double frees

    Default = NullDeallocCheck | OwnershipCheck | StackCheck | SizeTracking,
    Release = Empty,
    Debug   = NullDeallocCheck | OwnershipCheck | StackCheck | SizeTracking | AllocationTracking | BoundsCheck,
};

MARK_AS_POLICY(StackAllocatorPolicy);

enum class PoolAllocatorPolicy : UInt32
{
    ALLOCATOR_POLICIES,

    NullDeallocCheck     = Bit(0), // Check if the pointer is null when deallocating
    OwnershipCheck       = Bit(1), // Check if the pointer is owned/allocated by the allocator that is deallocating it
    DoubleFreePrevention = Bit(3), // Set the ptr to null on free to prevent double frees
    Growable             = Bit(4), // Allow the allocator to grow when memory is exhausted
    AllocationSizeCheck  = Bit(5), // Check if the size of object being allocated or deallocated is equal to objectSize

    Default = NullDeallocCheck | OwnershipCheck | SizeTracking | DoubleFreePrevention | AllocationSizeCheck,
    Release = Empty,
    Debug   = NullDeallocCheck | OwnershipCheck | SizeTracking | AllocationTracking | DoubleFreePrevention | AllocationSizeCheck,
};

MARK_AS_POLICY(PoolAllocatorPolicy);

enum class LinearAllocatorPolicy : UInt32
{
    ALLOCATOR_POLICIES,

    Growable  = Bit(0), // Allow the allocator to grow when memory is exhausted
    SizeCheck = Bit(1), // Check if the allocator has sufficient space when allocating //

    Default = SizeTracking | SizeCheck,
    Release = Empty,
    Debug   = SizeTracking | SizeCheck | AllocationTracking,
};

MARK_AS_POLICY(LinearAllocatorPolicy);

enum class MallocatorPolicy : UInt32
{
    BASE_ALLOCATOR_POLICIES,

    NullAllocCheck       = Bit(0), // Check if malloc returns null
    NullDeallocCheck     = Bit(1), // Check if the pointer is null when deallocating
    DoubleFreePrevention = Bit(2), // Set the ptr to null on free to prevent double frees

    Default = NullDeallocCheck | NullAllocCheck | SizeTracking | DoubleFreePrevention,
    Release = Empty,
    Debug   = NullDeallocCheck | NullAllocCheck | SizeTracking | AllocationTracking | DoubleFreePrevention,
};

MARK_AS_POLICY(MallocatorPolicy);

enum class LocalAllocatorPolicy : UInt32
{
    BASE_ALLOCATOR_POLICIES,

    NullAllocCheck       = Bit(0), // Check if malloc returns null
    NullDeallocCheck     = Bit(1), // Check if the pointer is null when deallocating
    DoubleFreePrevention = Bit(2), // Set the ptr to null on free to prevent double frees

    Default = NullDeallocCheck | NullAllocCheck | SizeTracking | DoubleFreePrevention,
    Release = Empty,
    Debug   = NullDeallocCheck | NullAllocCheck | SizeTracking | AllocationTracking | DoubleFreePrevention,
};

MARK_AS_POLICY(LocalAllocatorPolicy);

enum class VirtualAllocatorPolicy : UInt32
{
    BASE_ALLOCATOR_POLICIES,

    NullAllocCheck       = Bit(0), // Check if malloc returns null
    NullDeallocCheck     = Bit(1), // Check if the pointer is null when deallocating
    DoubleFreePrevention = Bit(2), // Set the ptr to null on free to prevent double frees

    Default = NullDeallocCheck | NullAllocCheck | SizeTracking | DoubleFreePrevention,
    Release = Empty,
    Debug   = NullDeallocCheck | NullAllocCheck | SizeTracking | AllocationTracking | DoubleFreePrevention,
};

MARK_AS_POLICY(VirtualAllocatorPolicy);

template <typename T>
concept AllocatorPolicy = requires(T a)
{
    T::Default;
    T::Release;
};

template <AllocatorPolicy Policy>
constexpr auto GetDefaultPolicy() -> Policy
{
#ifdef MEMARENA_DEBUG
    return Policy::Default;
#else
    return Policy::Release;
#endif
}

} // namespace Memarena