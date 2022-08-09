#pragma once

#include "../Aliases.hpp"
#include "../TypeAliases.hpp"

namespace Memarena
{

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
bool    IsAlignmentValid(int alignment);

} // namespace Memarena
