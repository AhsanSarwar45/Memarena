#include <gtest/gtest.h>

#include <Memarena/Memarena.hpp>

#include "Macro.hpp"
#include "MemoryTestObjects.hpp"
#include "Source/Policies/Policies.hpp"

using namespace Memarena;

TEST(PolicyTest, CombinePolicy){StackAllocatorPolicy policy = }

TEST(AlignmentTest, IsAlignmentValid)
{
    EXPECT_EQ(IsAlignmentValid(0), false);
    EXPECT_EQ(IsAlignmentValid(1), true);
    EXPECT_EQ(IsAlignmentValid(2), true);
    EXPECT_EQ(IsAlignmentValid(3), false);
    EXPECT_EQ(IsAlignmentValid(4), true);
    EXPECT_EQ(IsAlignmentValid(-1), false);
}