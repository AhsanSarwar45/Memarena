#include <gtest/gtest.h>

#include <Memarena/Memarena.hpp>

#include "Macro.hpp"
#include "MemoryTestObjects.hpp"

using namespace Memarena;

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

TEST(AlignmentTest, IsAlignmentValid)
{
    EXPECT_EQ(IsAlignmentValid(0), false);
    EXPECT_EQ(IsAlignmentValid(1), true);
    EXPECT_EQ(IsAlignmentValid(2), true);
    EXPECT_EQ(IsAlignmentValid(3), false);
    EXPECT_EQ(IsAlignmentValid(4), true);
    EXPECT_EQ(IsAlignmentValid(-1), false);
}