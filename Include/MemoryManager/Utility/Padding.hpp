#pragma once

#include "../TypeAliases.hpp"

namespace Memory
{

const Padding CalculatePadding(const Size baseAddress, const Size alignment);
const Padding CalculatePaddingWithHeader(const Size baseAddress, const Size alignment, const Size headerSize);

} // namespace Memory
