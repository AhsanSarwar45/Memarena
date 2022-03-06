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

enum class BoundsCheckPolicy
{
    None,
    Basic
};

enum class StackCheckPolicy
{
    None,
    Check
};

enum class NullCheckPolicy
{
    None,
    Check
};

enum class OwnershipCheckPolicy
{
    None,
    Check
};

enum class SizeCheckPolicy
{
    None,
    Check
};

struct StackAllocatorPolicy
{
    BoundsCheckPolicy    boundsCheckPolicy    = BoundsCheckPolicy::None;
    StackCheckPolicy     stackCheckPolicy     = StackCheckPolicy::None;
    OwnershipCheckPolicy ownershipCheckPolicy = OwnershipCheckPolicy::None;
    NullCheckPolicy      nullCheckPolicy      = NullCheckPolicy::None;
    SizeCheckPolicy      sizeCheckPolicy      = SizeCheckPolicy::None;

    constexpr StackAllocatorPolicy(BoundsCheckPolicy    _boundsCheckPolicy    = BoundsCheckPolicy::None,
                                   StackCheckPolicy     _stackCheckPolicy     = StackCheckPolicy::None,
                                   OwnershipCheckPolicy _ownershipCheckPolicy = OwnershipCheckPolicy::None,
                                   NullCheckPolicy      _nullCheckPolicy      = NullCheckPolicy::None,
                                   SizeCheckPolicy      _sizeCheckPolicy      = SizeCheckPolicy::None)
        : boundsCheckPolicy(_boundsCheckPolicy), stackCheckPolicy(_stackCheckPolicy), ownershipCheckPolicy(_ownershipCheckPolicy),
          nullCheckPolicy(_nullCheckPolicy), sizeCheckPolicy(_sizeCheckPolicy)
    {
    }
    constexpr StackAllocatorPolicy(StackCheckPolicy _stackCheckPolicy) : stackCheckPolicy(_stackCheckPolicy) {}
    constexpr StackAllocatorPolicy(OwnershipCheckPolicy _ownershipCheckPolicy) : ownershipCheckPolicy(_ownershipCheckPolicy) {}
    constexpr StackAllocatorPolicy(NullCheckPolicy _nullCheckPolicy) : nullCheckPolicy(_nullCheckPolicy) {}
    constexpr StackAllocatorPolicy(SizeCheckPolicy _sizeCheckPolicy) : sizeCheckPolicy(_sizeCheckPolicy) {}
};

} // namespace Memarena