#pragma once

#include "Aliases.hpp"

namespace Memory
{
struct AllocatorData
{
    std::unordered_map<std::string, Size> allocations;
    std::string                           debugName;
    Size                                  totalSize = 0;
    Size                                  usedSize  = 0;

    AllocatorData(const char* debugName, Size totalSize) : debugName(debugName), totalSize(totalSize) {}
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
} // namespace Memory