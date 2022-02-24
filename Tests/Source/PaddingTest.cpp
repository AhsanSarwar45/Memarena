#include <gtest/gtest.h>

#include <MemoryManager/StackAllocator.hpp>
#include <MemoryManager/Utility/Padding.hpp>

#include "Macro.hpp"
#include "MemoryTestObjects.hpp"

using namespace Memory;

// TEST(PaddingTest, CalculatePadding) { printf("%d", CalculatePadding(40, 1)); }