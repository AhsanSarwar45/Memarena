#include "PCH.hpp"

#include "BoundsChecking.hpp"

#include "Assert.hpp"

namespace Memarena
{

void BasicBoundsChecking::Guard(const UIntPtr address, const Offset offset, const Offset allocationSize)
{
    UIntPtr frontGuardAddress = address - sizeof(BoundGuardFront);
    UIntPtr backGuardAddress  = address + allocationSize;

    new (reinterpret_cast<void*>(frontGuardAddress)) BoundGuardFront(offset, allocationSize);
    new (reinterpret_cast<void*>(backGuardAddress)) BoundGuardBack(offset);
}

void BasicBoundsChecking::Check(const UIntPtr address, const Offset offset, const std::string& allocatorDebugName)
{
    UIntPtr          frontGuardAddress = address - sizeof(BoundGuardFront);
    BoundGuardFront* frontGuard        = reinterpret_cast<BoundGuardFront*>(frontGuardAddress);

    UIntPtr         backGuardAddress = frontGuardAddress + frontGuard->allocationSize;
    BoundGuardBack* backGuard        = reinterpret_cast<BoundGuardBack*>(frontGuardAddress);

    MEMARENA_ASSERT(frontGuard->offset == offset && backGuard->offset == offset,
                    "Error: Memory stomping detected in allocator %s at offset %d and address %d!\n", allocatorDebugName, offset, address);
}
} // namespace Memarena
