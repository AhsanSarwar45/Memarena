#pragma once

#include <cstdint>

namespace Memarena
{
using Byte = uint8_t;
using Size = size_t;

using UInt8   = uint8_t;
using UInt16  = uint16_t;
using UInt32  = uint32_t;
using UInt64  = uint64_t;
using UIntPtr = uintptr_t;

using Int8  = int8_t;
using Int16 = int16_t;
using Int32 = int32_t;
using Int64 = int64_t;

using ULLInt = unsigned long long int;

namespace SizeLiterals
{
constexpr inline Size operator"" _KiB(ULLInt x) { return 1024 * x; }
constexpr inline Size operator"" _MiB(ULLInt x) { return 1048576 * x; }
constexpr inline Size operator"" _GiB(ULLInt x) { return 1073741824 * x; }

constexpr inline Size operator"" _KB(ULLInt x) { return 1000 * x; }
constexpr inline Size operator"" _MB(ULLInt x) { return 1000000 * x; }
constexpr inline Size operator"" _GB(ULLInt x) { return 1000000000 * x; }
} // namespace SizeLiterals

} // namespace Memarena