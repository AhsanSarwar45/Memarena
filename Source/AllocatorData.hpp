#pragma once

#include <string>
#include <unordered_map>

#include "Aliases.hpp"

namespace Memarena
{
struct AllocatorData
{
    std::unordered_map<std::string, Size> allocations;
    std::string                           debugName;
    Size                                  totalSize = 0;
    Size                                  usedSize  = 0;
    Size                                  peakUsage = 0;

    AllocatorData(const std::string& _debugName, Size _totalSize) : debugName(_debugName), totalSize(_totalSize) {}
};

enum class ResizePolicy : UInt8
{
    Fixed,
    Resizable
};

} // namespace Memarena