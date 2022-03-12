
#include "StackAllocatorUtils.hpp"

namespace Memarena::Internal
{

Offset GetArrayEndOffset(const UIntPtr ptrAddress, const UIntPtr startAddress, const Offset objectCount, const Size objectSize)
{
    const Offset addressOffset = ptrAddress - startAddress;
    return addressOffset + (objectCount * objectSize);
}

} // namespace Memarena::Internal
