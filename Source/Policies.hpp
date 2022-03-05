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

    BoundGuardBack(Offset _offset) : offset(_offset) {}
};

enum class BoundsCheckingPolicy
{
    None,
    Basic
};

enum class StackAllocationPolicy
{
    Unsafe,
    Safe
};

} // namespace Memarena