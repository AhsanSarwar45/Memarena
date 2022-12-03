#include "./Alignment.hpp"

#include "Assert.hpp"

namespace Memarena
{

Alignment::Alignment(Size alignment)
{
    MEMARENA_DEFAULT_ASSERT(IsAlignmentValid(alignment), "Invalid alignment %d. Alignment in  must be a power of 2 and not equal to 0!",
                            alignment)
    value = alignment;
}

UIntPtr CalculateAlignedAddress(const UIntPtr baseAddress, const Alignment& alignment)
{
    return (baseAddress + (alignment - 1)) & ~(alignment - 1);
}

Padding CalculateShortestAlignedPadding(const UIntPtr baseAddress, const Alignment& alignment)
{
    return CalculateAlignedAddress(baseAddress, alignment) - baseAddress;
}

Padding CalculateAlignedPaddingWithHeader(const UIntPtr baseAddress, const Alignment& alignment, const Size headerSize)
{
    UInt8 padding = CalculateShortestAlignedPadding(baseAddress, alignment);

    return ExtendPaddingForHeader(padding, alignment, headerSize);
}

Padding ExtendPaddingForHeader(const Padding padding, const Alignment& alignment, const Size headerSize)
{
    Padding newPadding = padding;

    Size neededSpace = headerSize;

    if (newPadding < neededSpace)
    {
        // Header does not fit - Calculate next aligned address that header fits
        neededSpace -= newPadding;

        // How many alignments I need to fit the header
        if (neededSpace % alignment > 0)
        {
            newPadding += alignment * (1 + (neededSpace / alignment));
        }
        else
        {
            newPadding += alignment * (neededSpace / alignment);
        }
    }

    return newPadding;
}

bool IsAlignmentValid(const int alignment) { return (alignment != 0) && ((alignment & (alignment - 1)) == 0); }
} // namespace Memarena