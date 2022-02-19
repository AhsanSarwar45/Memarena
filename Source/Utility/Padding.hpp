#pragma once

#include "Aliases.hpp"

namespace Memory
{

const UInt8 CalculatePadding(const Size baseAddress, const Size alignment);
const UInt8 CalculatePaddingWithHeader(const Size baseAddress, const Size alignment, const Size headerSize);

} // namespace Memory
