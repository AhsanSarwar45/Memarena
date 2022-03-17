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

#define ALLOCATOR_POLICIES Empty = 0, SizeCheck = Bit(30), Multithreaded = Bit(29), UsageTracking = Bit(28), AllocationTracking = Bit(27)

enum class AllocatorPolicy : UInt32
{
    ALLOCATOR_POLICIES,

    Mask = SizeCheck | Multithreaded | UsageTracking | AllocationTracking
};

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

    NullCheck      = Bit(0),
    OwnershipCheck = Bit(1),
    BoundsCheck    = Bit(2),
    StackCheck     = Bit(3),

    Default = NullCheck | OwnershipCheck | SizeCheck | StackCheck | UsageTracking,
    Release = Empty,
    Debug   = NullCheck | OwnershipCheck | SizeCheck | StackCheck | UsageTracking | AllocationTracking | BoundsCheck,
};

MARK_AS_POLICY(StackAllocatorPolicy);

enum class PoolAllocatorPolicy : UInt32
{
    ALLOCATOR_POLICIES,

    NullCheck      = Bit(0),
    OwnershipCheck = Bit(1),
    BoundsCheck    = Bit(2),

    Default = NullCheck | OwnershipCheck | SizeCheck | UsageTracking,
    Release = Empty,
    Debug   = NullCheck | OwnershipCheck | SizeCheck | UsageTracking | AllocationTracking | BoundsCheck,
};

MARK_AS_POLICY(PoolAllocatorPolicy);

enum class LinearAllocatorPolicy : UInt32
{
    ALLOCATOR_POLICIES,

    Default = SizeCheck | UsageTracking,
    Release = Empty,
    Debug   = SizeCheck | UsageTracking | AllocationTracking,
};

MARK_AS_POLICY(LinearAllocatorPolicy);

} // namespace Memarena