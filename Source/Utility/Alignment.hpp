#pragma once

#include "../TypeAliases.hpp"

namespace Memarena
{

struct AlignOf
{
    Size value;

    explicit AlignOf(Size _value) : value(_value) {}
};

class Alignment
{
  public:
    Alignment(int alignment);
    Alignment(AlignOf alignOf);

    operator UInt8() const noexcept { return value; }

  private:
    UInt8 value;
};

UIntPtr CalculateAlignedAddress(const UIntPtr baseAddress, const Alignment& alignment);
Padding CalculateShortestAlignedPadding(const UIntPtr baseAddress, const Alignment& alignment);
Padding CalculateAlignedPaddingWithHeader(const UIntPtr baseAddress, const Alignment& alignment, const Size headerSize);
bool    IsAlignmentValid(const int alignment);

} // namespace Memarena
