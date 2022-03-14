#pragma once

#include <string>

#include "Source/Utility/Enums.hpp"

namespace Memarena
{

enum class AllocatorPolicy : UInt32
{
    SizeCheck     = Bit(0),
    Multithreaded = Bit(1),
};

template <typename Enum>
constexpr bool PolicyContains(Enum policy, Enum value)
{
    using UnderlyingType = typename std::underlying_type<Enum>::type;
    return static_cast<UnderlyingType>(policy) & static_cast<UnderlyingType>(value);
}

template <typename Enum>
constexpr bool PolicyContains(Enum policy, AllocatorPolicy value)
{
    using UnderlyingType = typename std::underlying_type<Enum>::type;
    return static_cast<UnderlyingType>(policy) & static_cast<UnderlyingType>(value);
}

template <typename Enum>
constexpr UInt32 PolicyToInt(Enum policy)
{
    return static_cast<UInt32>(policy);
}

enum class StackAllocatorPolicy : UInt32
{
    Empty          = 0,
    SizeCheck      = PolicyToInt(AllocatorPolicy::SizeCheck),
    Multithreaded  = PolicyToInt(AllocatorPolicy::Multithreaded),
    NullCheck      = Bit(2),
    OwnershipCheck = Bit(3),
    BoundsCheck    = Bit(4),
    StackCheck     = Bit(5),

    Default = NullCheck | OwnershipCheck | SizeCheck,
    Release = Empty,
    Debug   = NullCheck | OwnershipCheck | SizeCheck | StackCheck | BoundsCheck,
};

ENABLE_BITMASK_OPERATORS(StackAllocatorPolicy)

enum class LinearAllocatorPolicy : UInt32
{
    Empty         = 0,
    SizeCheck     = PolicyToInt(AllocatorPolicy::SizeCheck),
    Multithreaded = PolicyToInt(AllocatorPolicy::Multithreaded),

    Default = SizeCheck,
    Release = Empty,
    Debug   = SizeCheck,
};

ENABLE_BITMASK_OPERATORS(LinearAllocatorPolicy)

} // namespace Memarena