#pragma once

#include "Source/TypeAliases.hpp"

namespace Memarena
{
namespace Internal
{

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
    const UIntPtr address = reinterpret_cast<UIntPtr>(ptr);

    const UIntPtr headerAddress = address - sizeof(Header);
    // Construct the header at 'headerAdress' using placement new operator
    void* headerPtr = reinterpret_cast<void*>(headerAddress);
    new (headerPtr) Header(std::forward<Args>(argList)...);
}

template <typename Header>
Header GetHeaderFromPtr(UIntPtr& address)
{
    const UIntPtr headerAddress = address - sizeof(Header);
    const Header* headerPtr     = reinterpret_cast<Header*>(headerAddress);
    address                     = headerAddress;

    return *headerPtr;
}

} // namespace Internal
} // namespace Memarena
