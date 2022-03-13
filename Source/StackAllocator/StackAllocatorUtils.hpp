#pragma once

#include "Source/Policies.hpp"
#include "Source/TypeAliases.hpp"
#include <bit>

namespace Memarena::Internal
{
Offset GetArrayEndOffset(UIntPtr ptrAddress, UIntPtr startAddress, Offset objectCount, Size objectSize);

template <Size headerSize, StackAllocatorPolicy policy>
consteval Size GetTotalHeaderSize()
{
    if constexpr (PolicyContains(policy, StackAllocatorPolicy::BoundsCheck))
    {
        return headerSize + sizeof(BoundGuardFront);
    }
    else
    {
        return headerSize;
    }
}

template <typename Object, typename... Args>
Object* ConstructArray(void* voidPtr, const Offset objectCount, Args&&... argList)
{
    // Call the placement new operator, which constructs the Object
    Object* firstPtr = new (voidPtr) Object(std::forward<Args>(argList)...);

    Object* currentPtr = firstPtr;
    Object* lastPtr    = firstPtr + (objectCount - 1);

    while (currentPtr <= lastPtr)
    {
        new (currentPtr++) Object(std::forward<Args>(argList)...);
    }

    return firstPtr;
}
template <typename Object>
void DestructArray(Object* ptr, const Offset objectCount)
{
    for (Size i = objectCount - 1; i-- > 0;)
    {
        ptr[i].~Object();
    }
}

template <typename Header, typename... Args>
void AllocateHeader(void* ptr, Args&&... argList)
{
    const UIntPtr address       = std::bit_cast<UIntPtr>(ptr);
    const UIntPtr headerAddress = address - sizeof(Header);

    void* headerPtr = std::bit_cast<void*>(headerAddress);
    new (headerPtr) Header(std::forward<Args>(argList)...);

    // Header  header    = Header(std::forward<Args>(argList)...);
    // Header* headerPtr = reinterpret_cast<Header*>(headerAddress);
    // memcpy(headerPtr, &header, sizeof(Header));
}

template <typename Header>
Header GetHeaderFromPtr(UIntPtr& address)
{
    const UIntPtr headerAddress = address - sizeof(Header);
    const Header* headerPtr     = std::bit_cast<Header*>(headerAddress);
    address                     = headerAddress;

    return *headerPtr;
}

} // namespace Memarena::Internal
