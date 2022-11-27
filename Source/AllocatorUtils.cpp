
#include "AllocatorUtils.hpp"

namespace Memarena::Internal
{

Offset GetArrayEndOffset(const UIntPtr ptrAddress, const UIntPtr startAddress, const Offset objectCount, const Size objectSize,
                         const Size footerSize)
{
    const Offset addressOffset = ptrAddress - startAddress;
    return addressOffset + (objectCount * objectSize) + footerSize;
}

} // namespace Memarena::Internal
