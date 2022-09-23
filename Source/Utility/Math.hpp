#pragma once

#include "Source/Traits.hpp"

namespace Memarena
{

template <Integral T, Integral U>
T RoundUpToMultiple(T numToRound, U multiple)
{
    T roundedNum = numToRound;
    if (multiple != 0)
    {
        int remainder = numToRound % multiple;
        if (remainder != 0)
        {
            roundedNum = numToRound + multiple - remainder;
        }
    }
    return roundedNum;
}
} // namespace Memarena