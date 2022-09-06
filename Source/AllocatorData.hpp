#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "TypeAliases.hpp"

namespace Memarena
{
struct AllocationData
{
    SourceLocation sourceLocation;
    std::string    category;
    Size           size = 0;
};

struct AllocatorData
{
    std::vector<AllocationData> allocations;
    std::string                 debugName;
    UInt32                      allocationCount   = 0;
    UInt32                      deallocationCount = 0;
    Size                        totalSize         = 0;
    Size                        usedSize          = 0;
    Size                        peakUsage         = 0;
    bool                        isBaseAllocator   = false;
};

} // namespace Memarena