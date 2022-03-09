
#include "StackAllocatorUtils.hpp"

namespace Memarena
{
namespace Internal
{

Offset GetArrayEndOffset(const UIntPtr ptrAddress, const UIntPtr startAddress, const Offset objectCount, const Size objectSize)
{
    const Offset addressOffset = ptrAddress - startAddress;
    return addressOffset + (objectCount * objectSize);
}

} // namespace Internal
} // namespace Memarena
