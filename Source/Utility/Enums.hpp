#pragma once

#include <type_traits>

#include "Source/TypeAliases.hpp"

namespace Memarena
{
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
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type constexpr operator~(Enum rhs)
{
    using UnderlyingType = typename std::underlying_type<Enum>::type;
    return static_cast<Enum>(~static_cast<UnderlyingType>(rhs));
}

template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type constexpr operator|=(Enum& lhs, Enum rhs)
{
    using UnderlyingType = typename std::underlying_type<Enum>::type;
    lhs                  = static_cast<Enum>(static_cast<UnderlyingType>(lhs) | static_cast<UnderlyingType>(rhs));

    return lhs;
}

template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type constexpr operator&=(Enum& lhs, Enum rhs)
{
    using UnderlyingType = typename std::underlying_type<Enum>::type;
    lhs                  = static_cast<Enum>(static_cast<UnderlyingType>(lhs) & static_cast<UnderlyingType>(rhs));

    return lhs;
}

template <typename Enum>
typename std::enable_if<EnableBitMaskOperators<Enum>::enable, Enum>::type constexpr operator^=(Enum& lhs, Enum rhs)
{
    using UnderlyingType = typename std::underlying_type<Enum>::type;
    lhs                  = static_cast<Enum>(static_cast<UnderlyingType>(lhs) ^ static_cast<UnderlyingType>(rhs));

    return lhs;
}

#define ENABLE_BITMASK_OPERATORS(x)      \
    template <>                          \
    struct EnableBitMaskOperators<x>     \
    {                                    \
        static const bool enable = true; \
    };

} // namespace Memarena
