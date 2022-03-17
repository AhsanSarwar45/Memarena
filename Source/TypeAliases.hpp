#pragma once

#include "Aliases.hpp"
#include <experimental/source_location>

namespace Memarena
{
using Padding = UInt8;
using Offset  = UInt32;

template <typename T>
using RawPtr = T*;

using SourceLocation = std::experimental::source_location;

// using BoundGuard = UInt32;
// using Alignment = UInt8;
} // namespace Memarena