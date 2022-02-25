#include <gtest/gtest.h>

#include <MemoryManager/StackAllocator.hpp>
#include <MemoryManager/Utility/ALignment.hpp>

#include "Macro.hpp"
#include "MemoryTestObjects.hpp"

using namespace Memory;

TEST(AlignmentTest, CalculateAlignedAddress)
{
    EXPECT_EQ(CalculateAlignedAddress(40, 4), 40);
    EXPECT_EQ(CalculateAlignedAddress(41, 4), 44);
    EXPECT_EQ(CalculateAlignedAddress(42, 4), 44);
    EXPECT_EQ(CalculateAlignedAddress(43, 4), 44);
    EXPECT_EQ(CalculateAlignedAddress(44, 4), 44);
    EXPECT_EQ(CalculateAlignedAddress(45, 1), 45);
    EXPECT_EQ(CalculateAlignedAddress(45, 2), 46);
    EXPECT_EQ(CalculateAlignedAddress(45, 16), 48);
    EXPECT_EQ(CalculateAlignedAddress(24, 8), 24);
    EXPECT_EQ(CalculateAlignedAddress(25, 8), 32);
}