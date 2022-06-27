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

#define ALLOCATOR_POLICIES                                                                                \
    Empty = 0, SizeCheck = Bit(30), /* Check if the allocator has sufficient space when allocating */     \
        Multithreaded = Bit(29),    /* Make allocations thread-safe. This will also make them blocking */ \
        UsageTracking = Bit(28),    /* Track the amount of space used by this allocator */                \
        AllocationTracking = Bit(27) /* Track the amount of allocations and deallocations of this allocator */ \  

enum class AllocatorPolicy : UInt32
{ALLOCATOR_POLICIES, Mask = SizeCheck | Multithreaded | UsageTracking | AllocationTracking};

MARK_AS_POLICY(AllocatorPolicy);

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

template <typename Policy>
constexpr AllocatorPolicy ToAllocatorPolicy(Policy policy)
{
    return static_cast<AllocatorPolicy>(policy) & AllocatorPolicy::Mask;
}

enum class StackAllocatorPolicy : UInt32
{
    ALLOCATOR_POLICIES,

    NullCheck      = Bit(0), // Check if the pointer is null when deallocating
    OwnershipCheck = Bit(1), // Check if the pointer is owned/allocated by the allocator that is deallocating it
    BoundsCheck    = Bit(2), // Check if an allocation overwrites another allocation
    StackCheck     = Bit(3), // Check is deallocations are performed in LIFO order

    Default = NullCheck | OwnershipCheck | SizeCheck | StackCheck | UsageTracking,
    Release = Empty,
    Debug   = NullCheck | OwnershipCheck | SizeCheck | StackCheck | UsageTracking | AllocationTracking | BoundsCheck,
};

MARK_AS_POLICY(StackAllocatorPolicy);

enum class PoolAllocatorPolicy : UInt32
{
    ALLOCATOR_POLICIES,

    NullCheck      = Bit(0), // Check if the pointer is null when deallocating
    OwnershipCheck = Bit(1), // Check if the pointer is owned/allocated by the allocator that is deallocating it
    BoundsCheck    = Bit(2), // Check if an allocation overwrites another allocation

    Default = NullCheck | OwnershipCheck | SizeCheck | UsageTracking,
    Release = Empty,
    Debug   = NullCheck | OwnershipCheck | SizeCheck | UsageTracking | AllocationTracking | BoundsCheck,
};

MARK_AS_POLICY(PoolAllocatorPolicy);

enum class LinearAllocatorPolicy : UInt32
{
    ALLOCATOR_POLICIES,

    Resizable = Bit(1), // Allow the allocator to be resized when memory is exhausted

    Default = SizeCheck | UsageTracking,
    Release = Empty,
    Debug   = SizeCheck | UsageTracking | AllocationTracking,
};

MARK_AS_POLICY(LinearAllocatorPolicy);

} // namespace Memarena