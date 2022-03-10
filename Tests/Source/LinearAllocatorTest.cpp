#include <gtest/gtest.h>

#include <Memarena/Memarena.hpp>

#include "Macro.hpp"
#include "MemoryTestObjects.hpp"

using namespace Memarena;
using namespace Memarena::SizeLiterals;

class LinearAllocatorTest : public ::testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}

    LinearAllocator<> linearAllocator = LinearAllocator<>(10_MB);
};

template <typename Object, typename... Args>
Object* CheckNewRaw(LinearAllocator<>& allocator, Args&&... argList)
{
    Object* object = allocator.NewRaw<Object>(std::forward<Args>(argList)...);

    EXPECT_EQ(*object, Object(std::forward<Args>(argList)...));

    return object;
}
template <typename Object, typename... Args>
Object* CheckNewArrayRaw(LinearAllocator<>& allocator, size_t objectCount, Args&&... argList)
{
    Object* arr = allocator.NewArrayRaw<Object>(objectCount, std::forward<Args>(argList)...);

    for (size_t i = 0; i < objectCount; i++)
    {
        EXPECT_EQ(arr[i], Object(std::forward<Args>(argList)...));
    }
    return arr;
}

TEST_F(LinearAllocatorTest, Initialize) { EXPECT_EQ(linearAllocator.GetUsedSize(), 0); }

TEST_F(LinearAllocatorTest, RawNewSingleObject) { CheckNewRaw<TestObject>(linearAllocator, 1, 2.1f, 'a', false, 10.6f); }

TEST_F(LinearAllocatorTest, RawNewMultipleObjects)
{
    for (size_t i = 0; i < 10; i++)
    {
        CheckNewRaw<TestObject>(linearAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }
    for (size_t i = 0; i < 10; i++)
    {
        CheckNewRaw<TestObject2>(linearAllocator, i, i + 1.5, i + 2.5, i % 2, Pair(1, 2.5f));
    }
}

TEST_F(LinearAllocatorTest, RawNewMixed)
{
    TestObject* arr1    = CheckNewArrayRaw<TestObject>(linearAllocator, 10, 1, 2.1f, 'a', false, 10.6f);
    TestObject* object1 = CheckNewRaw<TestObject>(linearAllocator, 1, 2.1f, 'a', false, 10.6f);
    TestObject* object2 = CheckNewRaw<TestObject>(linearAllocator, 1, 2.1f, 'a', false, 10.6f);
    TestObject* arr2    = CheckNewArrayRaw<TestObject>(linearAllocator, 10, 1, 2.1f, 'a', false, 10.6f);
}

TEST_F(LinearAllocatorTest, Reset)
{
    LinearAllocator<> linearAllocator2 = LinearAllocator<>(10 * sizeof(TestObject));
    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckNewRaw<TestObject>(linearAllocator2, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }

    linearAllocator2.Reset();

    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckNewRaw<TestObject>(linearAllocator2, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }
}

TEST_F(LinearAllocatorTest, GetUsedSizeNew)
{
    const int numObjects = 10;
    for (size_t i = 0; i < numObjects; i++)
    {
        TestObject* object = CheckNewRaw<TestObject>(linearAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }

    EXPECT_EQ(linearAllocator.GetUsedSize(), numObjects * (sizeof(TestObject)));
}

TEST_F(LinearAllocatorTest, GetUsedSizeNewArray)
{
    const int numObjects = 10;

    TestObject* arr = CheckNewArrayRaw<TestObject>(linearAllocator, numObjects, 1, 2.1f, 'a', false, 10.6f);

    EXPECT_EQ(linearAllocator.GetUsedSize(), std::max(alignof(TestObject), numObjects * sizeof(TestObject)));
}

#ifdef MEMARENA_ENABLE_ASSERTS

class LinearAllocatorDeathTest : public ::testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}

    LinearAllocator<> linearAllocator = LinearAllocator<>(10_MB);
};

TEST_F(LinearAllocatorDeathTest, MaxSizeAllocation)
{
    // TODO Write proper exit messages
    ASSERT_DEATH({ LinearAllocator<> linearAllocator2 = LinearAllocator<>(std::numeric_limits<Offset>::max() + 1); }, ".*");
}

TEST_F(LinearAllocatorDeathTest, NewOutOfMemory)
{
    LinearAllocator linearAllocator2{10};

    // TODO Write proper exit messages
    ASSERT_DEATH({ TestObject* object = linearAllocator2.NewRaw<TestObject>(1, 2.1f, 'a', false, 10.6f); }, ".*");
}

#endif
