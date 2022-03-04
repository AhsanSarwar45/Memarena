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

// class NoBoundsChecking
// {

//   public:
//     inline void Guard(const UIntPtr address, const Offset offset, const Offset allocationSize) {}
//     inline void Check(const UIntPtr address, const Offset offset, const std::string& allocatorDebugName) {}

//   public:
//     static const Size s_FrontGuardSize = 0;
// };

// class BasicBoundsChecking
// {
//   public:
//     void Guard(const UIntPtr address, const Offset offset, Offset allocationSize);
//     void Check(const UIntPtr address, const Offset offset, const std::string& allocatorDebugName);

//   public:
//     static const Size s_FrontGuardSize = sizeof(BoundGuardFront);
// };
} // namespace Memarena