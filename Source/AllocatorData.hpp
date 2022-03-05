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

struct Chunk
{
    /*
    When a chunk is free, the `next` contains the
    address of the next chunk in a list.

    When it's allocated, this space is used by
    the user.
    */
    Chunk* next;
};
} // namespace Memarena