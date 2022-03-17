#include <gtest/gtest.h>

#include <Memarena/Memarena.hpp>

#include "MemoryTestObjects.hpp"

using namespace Memarena::SizeLiterals;

using namespace Memarena;

class MemoryTrackerTest : public ::testing::Test
{
  protected:
    void SetUp() override { MemoryTracker::Reset(); }
    void TearDown() override {}
};

TEST_F(MemoryTrackerTest, SingleAllocator)
{
    constexpr StackAllocatorPolicy policy = StackAllocatorPolicy::Default | StackAllocatorPolicy::AllocationTracking;

    StackAllocator<policy> stackAllocator{10_MB};

    int* num = static_cast<int*>(stackAllocator.Allocate<int>("Testing/StackAllocator"));

    const AllocatorVector allocators = MemoryTracker::GetAllocators();

    EXPECT_EQ(allocators.size(), 1);
    EXPECT_EQ(allocators[0]->totalSize, 10_MB);
    EXPECT_EQ(allocators[0]->allocationCount, 1);
    EXPECT_EQ(allocators[0]->allocations[0].category, std::string("Testing/StackAllocator"));
    EXPECT_EQ(allocators[0]->allocations[0].size, sizeof(int));
}