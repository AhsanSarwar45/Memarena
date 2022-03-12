#pragma once

#include "Aliases.hpp"

namespace Memarena
{
using Padding = UInt8;
using Offset  = UInt32;

template <typename T>
using RawPtr = T*;
// using BoundGuard = UInt32;
// using Alignment = UInt8;
} // namespace Memarena