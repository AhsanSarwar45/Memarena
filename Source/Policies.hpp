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
    BoundsCheckPolicy    boundsCheckPolicy;
    StackCheckPolicy     stackCheckPolicy;
    OwnershipCheckPolicy ownershipCheckPolicy;
    NullCheckPolicy      nullCheckPolicy;
    SizeCheckPolicy      sizeCheckPolicy;

    constexpr StackAllocatorPolicy(BoundsCheckPolicy    _boundsCheckPolicy    = BoundsCheckPolicy::None,
                                   StackCheckPolicy     _stackCheckPolicy     = StackCheckPolicy::None,
                                   OwnershipCheckPolicy _ownershipCheckPolicy = OwnershipCheckPolicy::None,
                                   NullCheckPolicy      _nullCheckPolicy      = NullCheckPolicy::None,
                                   SizeCheckPolicy      _sizeCheckPolicy      = SizeCheckPolicy::None)
        : boundsCheckPolicy(_boundsCheckPolicy), stackCheckPolicy(_stackCheckPolicy), ownershipCheckPolicy(_ownershipCheckPolicy),
          nullCheckPolicy(_nullCheckPolicy), sizeCheckPolicy(_sizeCheckPolicy)
    {
    }

    constexpr StackAllocatorPolicy(StackCheckPolicy _stackCheckPolicy, BoundsCheckPolicy _boundsCheckPolicy = BoundsCheckPolicy::None,
                                   OwnershipCheckPolicy _ownershipCheckPolicy = OwnershipCheckPolicy::None,
                                   NullCheckPolicy      _nullCheckPolicy      = NullCheckPolicy::None,
                                   SizeCheckPolicy      _sizeCheckPolicy      = SizeCheckPolicy::None)
        : boundsCheckPolicy(_boundsCheckPolicy), stackCheckPolicy(_stackCheckPolicy), ownershipCheckPolicy(_ownershipCheckPolicy),
          nullCheckPolicy(_nullCheckPolicy), sizeCheckPolicy(_sizeCheckPolicy)
    {
    }
    constexpr StackAllocatorPolicy(OwnershipCheckPolicy _ownershipCheckPolicy, StackCheckPolicy _stackCheckPolicy = StackCheckPolicy::None,
                                   BoundsCheckPolicy _boundsCheckPolicy = BoundsCheckPolicy::None,
                                   NullCheckPolicy   _nullCheckPolicy   = NullCheckPolicy::None,
                                   SizeCheckPolicy   _sizeCheckPolicy   = SizeCheckPolicy::None)
        : boundsCheckPolicy(_boundsCheckPolicy), stackCheckPolicy(_stackCheckPolicy), ownershipCheckPolicy(_ownershipCheckPolicy),
          nullCheckPolicy(_nullCheckPolicy), sizeCheckPolicy(_sizeCheckPolicy)
    {
    }
    constexpr StackAllocatorPolicy(NullCheckPolicy _nullCheckPolicy, StackCheckPolicy _stackCheckPolicy = StackCheckPolicy::None,
                                   BoundsCheckPolicy    _boundsCheckPolicy    = BoundsCheckPolicy::None,
                                   OwnershipCheckPolicy _ownershipCheckPolicy = OwnershipCheckPolicy::None,
                                   SizeCheckPolicy      _sizeCheckPolicy      = SizeCheckPolicy::None)
        : boundsCheckPolicy(_boundsCheckPolicy), stackCheckPolicy(_stackCheckPolicy), ownershipCheckPolicy(_ownershipCheckPolicy),
          nullCheckPolicy(_nullCheckPolicy), sizeCheckPolicy(_sizeCheckPolicy)
    {
    }
    constexpr StackAllocatorPolicy(SizeCheckPolicy _sizeCheckPolicy, StackCheckPolicy _stackCheckPolicy = StackCheckPolicy::None,
                                   BoundsCheckPolicy    _boundsCheckPolicy    = BoundsCheckPolicy::None,
                                   OwnershipCheckPolicy _ownershipCheckPolicy = OwnershipCheckPolicy::None,
                                   NullCheckPolicy      _nullCheckPolicy      = NullCheckPolicy::None)
        : boundsCheckPolicy(_boundsCheckPolicy), stackCheckPolicy(_stackCheckPolicy), ownershipCheckPolicy(_ownershipCheckPolicy),
          nullCheckPolicy(_nullCheckPolicy), sizeCheckPolicy(_sizeCheckPolicy)
    {
    }
};

} // namespace Memarena