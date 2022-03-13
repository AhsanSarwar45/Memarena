#pragma once

#include <string>

#include "TypeAliases.hpp"

namespace Memarena
{
struct BoundGuardFront
{
    Offset offset;
    Offset allocationSize;

    BoundGuardFront(Offset _offset, Offset _allocationSize) : offset(_offset), allocationSize(_allocationSize) {}
};

struct BoundGuardBack
{
    Offset offset;

    explicit BoundGuardBack(Offset _offset) : offset(_offset) {}
};

constexpr int Bit(int x) { return 1 << x; }

template <typename Enum>
struct EnableBitMaskOperators
{
    static const bool enable = false;
};

template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type constexpr operator|(Enum lhs, Enum rhs)
{
    using UnderlyingType = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(static_cast<UnderlyingType>(lhs) | static_cast<UnderlyingType>(rhs));
}

template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type constexpr operator&(Enum lhs, Enum rhs)
{
    using UnderlyingType = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(static_cast<UnderlyingType>(lhs) & static_cast<UnderlyingType>(rhs));
}

template <typename Enum>
constexpr bool PolicyContains(Enum policy, Enum value)
{
    using UnderlyingType = typename std::underlying_type<Enum>::type;
    return static_cast<UnderlyingType>(policy) & static_cast<UnderlyingType>(value);
}

#define ENABLE_BITMASK_OPERATORS(x)      \
    template <>                          \
    struct EnableBitMaskOperators<x>     \
    {                                    \
        static const bool enable = true; \
    };

enum class StackAllocatorPolicy : UInt32
{
    Empty          = 0,
    NullCheck      = Bit(0),
    OwnershipCheck = Bit(1),
    SizeCheck      = Bit(2),
    BoundsCheck    = Bit(3),
    MultiThreaded  = Bit(4),
    StackCheck     = Bit(5),

    Default  = NullCheck | OwnershipCheck | SizeCheck,
    Release  = Empty,
    RelDebug = NullCheck | OwnershipCheck | SizeCheck | StackCheck,
    Debug    = NullCheck | OwnershipCheck | SizeCheck | StackCheck | BoundsCheck,
};

ENABLE_BITMASK_OPERATORS(StackAllocatorPolicy)

enum class LinearAllocatorPolicy : UInt32
{
    Empty         = 0,
    SizeCheck     = Bit(0),
    MultiThreaded = Bit(1),

    Default  = SizeCheck,
    Release  = Empty,
    RelDebug = SizeCheck,
    Debug    = SizeCheck,
};

ENABLE_BITMASK_OPERATORS(LinearAllocatorPolicy)

} // namespace Memarena