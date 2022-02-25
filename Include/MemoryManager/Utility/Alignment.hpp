#pragma once

#include "../TypeAliases.hpp"

namespace Memory
{

UIntPtr CalculateAlignedAddress(const UIntPtr baseAddress, const Alignment alignment);
Padding CalculateShortestAlignedPadding(const UIntPtr baseAddress, const Alignment alignment);
Padding CalculateAlignedPaddingWithHeader(const UIntPtr baseAddress, const Alignment alignment, const Size headerSize);

} // namespace Memory
