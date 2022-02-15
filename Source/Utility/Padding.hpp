#pragma once

#include "Aliases.hpp"

namespace Memory
{

const Size CalculatePadding(const Size baseAddress, const Size alignment);
const Size CalculatePaddingWithHeader(const Size baseAddress, const Size alignment, const Size headerSize);

} // namespace Memory
