#include "Utility/Alignment.hpp"

namespace Memory
{
UIntPtr CalculateAlignedAddress(const UIntPtr baseAddress, const Alignment alignment)
{
    return (baseAddress + (alignment - 1)) & ~(alignment - 1);
}

Padding CalculateShortestAlignedPadding(const UIntPtr baseAddress, const Alignment alignment)
{
    const Size    multiplier     = (baseAddress / alignment) + 1;
    const UIntPtr alignedAddress = multiplier * alignment;
    const UInt8   padding        = alignedAddress - baseAddress;
    return padding;
}

Padding CalculateAlignedPaddingWithHeader(const UIntPtr baseAddress, const Alignment alignment, const Size headerSize)
{

    UInt8 padding = CalculateShortestAlignedPadding(baseAddress, alignment);

    Size neededSpace = headerSize;

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