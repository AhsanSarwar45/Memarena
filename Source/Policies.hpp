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

struct AllocatorPolicy
{
  public:
    OwnershipCheckPolicy ownershipCheckPolicy = OwnershipCheckPolicy::Check;
    NullCheckPolicy      nullCheckPolicy      = NullCheckPolicy::Check;
    SizeCheckPolicy      sizeCheckPolicy      = SizeCheckPolicy::Check;
    BoundsCheckPolicy    boundsCheckPolicy    = BoundsCheckPolicy::None;

  protected:
    constexpr AllocatorPolicy(BoundsCheckPolicy _boundsCheckPolicy, OwnershipCheckPolicy _ownershipCheckPolicy,
                              NullCheckPolicy _nullCheckPolicy, SizeCheckPolicy _sizeCheckPolicy)
        : ownershipCheckPolicy(_ownershipCheckPolicy), nullCheckPolicy(_nullCheckPolicy), sizeCheckPolicy(_sizeCheckPolicy),
          boundsCheckPolicy(_boundsCheckPolicy)
    {
    }

    constexpr AllocatorPolicy(OwnershipCheckPolicy _ownershipCheckPolicy) : ownershipCheckPolicy(_ownershipCheckPolicy) {}
    constexpr AllocatorPolicy(NullCheckPolicy _nullCheckPolicy) : nullCheckPolicy(_nullCheckPolicy) {}
    constexpr AllocatorPolicy(SizeCheckPolicy _sizeCheckPolicy) : sizeCheckPolicy(_sizeCheckPolicy) {}
    constexpr AllocatorPolicy(BoundsCheckPolicy _boundsCheckPolicy) : boundsCheckPolicy(_boundsCheckPolicy) {}

    constexpr AllocatorPolicy() {}
};

struct LinearAllocatorPolicy : public AllocatorPolicy
{
    constexpr LinearAllocatorPolicy(BoundsCheckPolicy    boundsCheckPolicy     = BoundsCheckPolicy::None,
                                    OwnershipCheckPolicy _ownershipCheckPolicy = OwnershipCheckPolicy::Check,
                                    NullCheckPolicy      _nullCheckPolicy      = NullCheckPolicy::Check,
                                    SizeCheckPolicy      _sizeCheckPolicy      = SizeCheckPolicy::Check)
        : AllocatorPolicy(boundsCheckPolicy, _ownershipCheckPolicy, _nullCheckPolicy, _sizeCheckPolicy)
    {
    }

    explicit constexpr LinearAllocatorPolicy(OwnershipCheckPolicy _ownershipCheckPolicy) : AllocatorPolicy(_ownershipCheckPolicy) {}
    explicit constexpr LinearAllocatorPolicy(NullCheckPolicy _nullCheckPolicy) : AllocatorPolicy(_nullCheckPolicy) {}
    explicit constexpr LinearAllocatorPolicy(SizeCheckPolicy _sizeCheckPolicy) : AllocatorPolicy(_sizeCheckPolicy) {}
};

struct StackAllocatorPolicy : public AllocatorPolicy
{
    StackCheckPolicy stackCheckPolicy = StackCheckPolicy::None;

    constexpr StackAllocatorPolicy(BoundsCheckPolicy    _boundsCheckPolicy    = BoundsCheckPolicy::None,
                                   StackCheckPolicy     _stackCheckPolicy     = StackCheckPolicy::None,
                                   OwnershipCheckPolicy _ownershipCheckPolicy = OwnershipCheckPolicy::Check,
                                   NullCheckPolicy      _nullCheckPolicy      = NullCheckPolicy::Check,
                                   SizeCheckPolicy      _sizeCheckPolicy      = SizeCheckPolicy::Check)
        : AllocatorPolicy(_boundsCheckPolicy, _ownershipCheckPolicy, _nullCheckPolicy, _sizeCheckPolicy),
          stackCheckPolicy(_stackCheckPolicy)
    {
    }
    explicit constexpr StackAllocatorPolicy(StackCheckPolicy _stackCheckPolicy) : stackCheckPolicy(_stackCheckPolicy) {}
    explicit constexpr StackAllocatorPolicy(OwnershipCheckPolicy _ownershipCheckPolicy) : AllocatorPolicy(_ownershipCheckPolicy) {}
    explicit constexpr StackAllocatorPolicy(NullCheckPolicy _nullCheckPolicy) : AllocatorPolicy(_nullCheckPolicy) {}
    explicit constexpr StackAllocatorPolicy(SizeCheckPolicy _sizeCheckPolicy) : AllocatorPolicy(_sizeCheckPolicy) {}
};

} // namespace Memarena