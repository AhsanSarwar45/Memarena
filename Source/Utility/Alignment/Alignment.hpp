#pragma once

#include <cstddef>

#include "Source/Aliases.hpp"
#include "Source/TypeAliases.hpp"

namespace Memarena
{

constexpr Size defaultAlignment = alignof(std::max_align_t);

class Alignment
{
  public:
    Alignment(Size alignment); // NOLINT

    operator UInt8() const noexcept { return value; } // NOLINT

  private:
    UInt8 value;
};

UIntPtr CalculateAlignedAddress(UIntPtr baseAddress, const Alignment& alignment);
Padding CalculateShortestAlignedPadding(UIntPtr baseAddress, const Alignment& alignment);
Padding CalculateAlignedPaddingWithHeader(UIntPtr baseAddress, const Alignment& alignment, Size headerSize);
Padding ExtendPaddingForHeader(Padding padding, const Alignment& alignment, Size headerSize);
bool    IsAlignmentValid(int alignment);

} // namespace Memarena
