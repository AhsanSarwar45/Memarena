#include "Padding.hpp"

namespace Memory
{
const UInt8 CalculatePadding(const Size baseAddress, const Size alignment)
{
    const Size  multiplier     = (baseAddress / alignment) + 1;
    const Size  alignedAddress = multiplier * alignment;
    const UInt8 padding        = alignedAddress - baseAddress;
    return padding;
}

const UInt8 CalculatePaddingWithHeader(const Size baseAddress, const Size alignment, const Size headerSize)
{
    UInt8 padding     = CalculatePadding(baseAddress, alignment);
    Size  neededSpace = headerSize;

    if (padding < neededSpace)
    {
        // Header does not fit - Calculate next aligned address that header fits
        neededSpace -= padding;

        // How many alignments I need to fit the header
        if (neededSpace % alignment > 0)
        {
            padding += alignment * (1 + (neededSpace / alignment));
        }
        else
        {
            padding += alignment * (neededSpace / alignment);
        }
    }

    return padding;
}
} // namespace Memory