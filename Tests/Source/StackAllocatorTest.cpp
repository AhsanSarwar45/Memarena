#include <gtest/gtest.h>

#include <Memarena/Memarena.hpp>

#include "Macro.hpp"
#include "MemoryTestObjects.hpp"

using namespace Memarena;
using namespace Memarena::SizeOperators;

class StackAllocatorTest : public ::testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}

    StackAllocator<> stackAllocator = StackAllocator<>(10_MB);
};

TestObject* CheckTestObjectNew(StackAllocator<>& stackAllocator, int a, float b, char c, bool d, float e)
{
    TestObject* object = stackAllocator.NewRaw<TestObject>(a, b, c, d, e);

    EXPECT_EQ(object->a, a);
    EXPECT_EQ(object->b, b);
    EXPECT_EQ(object->c, c);
    EXPECT_EQ(object->d, d);
    EXPECT_EQ(object->e, e);

    return object;
}

// template <BoundsCheckingPolicy boundsCheckingPolicy>
// TestObject* CheckTestObjectNew(StackAllocator<boundsCheckingPolicy>& stackAllocator, int a, float b, char c, bool d, float e)
// {
//     TestObject* object = stackAllocator.NewRaw<TestObject>(a, b, c, d, e);

//     EXPECT_EQ(object->a, a);
//     EXPECT_EQ(object->b, b);
//     EXPECT_EQ(object->c, c);
//     EXPECT_EQ(object->d, d);
//     EXPECT_EQ(object->e, e);

//     return object;
// }

TestObject2* CheckTestObjectNew2(StackAllocator<>& stackAllocator, int a, double b, double c, bool d, std::vector<int> e)
{
    TestObject2* object = stackAllocator.NewRaw<TestObject2>(a, b, c, d, e);

    EXPECT_EQ(object->a, a);
    EXPECT_EQ(object->b, b);
    EXPECT_EQ(object->c, c);
    EXPECT_EQ(object->d, d);
    EXPECT_EQ(object->e.size(), e.size());

    return object;
}

// template <BoundsCheckingPolicy boundsCheckingPolicy>
// TestObject2* CheckTestObjectNew2(StackAllocator<boundsCheckingPolicy>& stackAllocator, int a, double b, double c, bool d,
//                                  std::vector<int> e)
// {
//     TestObject2* object = stackAllocator.NewRaw<TestObject2>(a, b, c, d, e);

//     EXPECT_EQ(object->a, a);
//     EXPECT_EQ(object->b, b);
//     EXPECT_EQ(object->c, c);
//     EXPECT_EQ(object->d, d);
//     EXPECT_EQ(object->e.size(), e.size());

//     return object;
// }
TestObject* CheckTestObjectNewArray(StackAllocator<>& stackAllocator, size_t objectCount)
{
    TestObject* arr = stackAllocator.NewArrayRaw<TestObject>(objectCount, 1, 2.1f, 'a', false, 10.6f);

    for (size_t i = 0; i < objectCount; i++)
    {
        EXPECT_EQ(arr[i].a, 1);
        EXPECT_EQ(arr[i].b, 2.1f);
        EXPECT_EQ(arr[i].c, 'a');
        EXPECT_EQ(arr[i].d, false);
        EXPECT_EQ(arr[i].e, 10.6f);
    }
    return arr;
}

TEST_F(StackAllocatorTest, Initialize) { EXPECT_EQ(stackAllocator.GetUsedSize(), 0); }

TEST_F(StackAllocatorTest, NewSingleObject) { CheckTestObjectNew(stackAllocator, 1, 2.1f, 'a', false, 10.6f); }

TEST_F(StackAllocatorTest, NewMultipleSameObjects)
{
    for (size_t i = 0; i < 10; i++)
    {
        CheckTestObjectNew(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }
}

TEST_F(StackAllocatorTest, NewMultipleDifferentObjects)
{
    for (size_t i = 0; i < 10; i++)
    {
        CheckTestObjectNew(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }
    for (size_t i = 0; i < 10; i++)
    {
        CheckTestObjectNew2(stackAllocator, i, i + 1.5, i + 2.5, i % 2, std::vector<int>(i));
    }
}

TEST_F(StackAllocatorTest, NewDeleteSingleObject)
{
    TestObject* object = CheckTestObjectNew(stackAllocator, 1, 2.1f, 'a', false, 10.6f);

    stackAllocator.Delete<TestObject>(object);
}

TEST_F(StackAllocatorTest, NewDeleteMultipleSameObjects)
{
    std::vector<TestObject*> objects;

    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckTestObjectNew(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);

        objects.push_back(object);
    }

    // Remember to delete in reverse order
    for (int i = objects.size() - 1; i >= 0; i--)
    {
        stackAllocator.Delete(objects[i]);
    }
}

TEST_F(StackAllocatorTest, NewDeleteMultipleDifferentObjects)
{
    std::vector<TestObject*>  objects1;
    std::vector<TestObject2*> objects2;

    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckTestObjectNew(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);

        objects1.push_back(object);
    }
    for (size_t i = 0; i < 10; i++)
    {
        TestObject2* object = CheckTestObjectNew2(stackAllocator, i, i + 1.5, i + 2.5, i % 2, std::vector<int>(i));

        objects2.push_back(object);
    }

    for (int i = objects2.size() - 1; i >= 0; i--)
    {
        stackAllocator.Delete(objects2[i]);
    }
    for (int i = objects1.size() - 1; i >= 0; i--)
    {
        stackAllocator.Delete(objects1[i]);
    }
}

TEST_F(StackAllocatorTest, NewDeleteNewSingleObject)
{
    TestObject* object = CheckTestObjectNew(stackAllocator, 1, 2.1f, 'a', false, 10.6f);

    stackAllocator.Delete(object);

    TestObject* object2 = CheckTestObjectNew(stackAllocator, 1, 2.1f, 'a', false, 10.6f);
}

TEST_F(StackAllocatorTest, NewDeleteNewMultipleSameObjects)
{
    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckTestObjectNew(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);

        stackAllocator.Delete(object);
    }
}

TEST_F(StackAllocatorTest, NewDeleteNewMultipleDifferentObjects)
{
    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckTestObjectNew(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);

        stackAllocator.Delete(object);
    }
    for (size_t i = 0; i < 10; i++)
    {
        TestObject2* object = CheckTestObjectNew2(stackAllocator, i, i + 1.5, i + 2.5, i % 2, std::vector<int>(i));

        stackAllocator.Delete(object);
    }
}

TEST_F(StackAllocatorTest, NewArrayRaw) { TestObject* arr = CheckTestObjectNewArray(stackAllocator, 10); }

TEST_F(StackAllocatorTest, NewDeleteArray)
{
    TestObject* arr = CheckTestObjectNewArray(stackAllocator, 10);
    stackAllocator.DeleteArray(arr);
}

TEST_F(StackAllocatorTest, NewMixed)
{
    TestObject* arr1    = CheckTestObjectNewArray(stackAllocator, 10);
    TestObject* object1 = CheckTestObjectNew(stackAllocator, 1, 2.1f, 'a', false, 10.6f);
    TestObject* object2 = CheckTestObjectNew(stackAllocator, 1, 2.1f, 'a', false, 10.6f);
    TestObject* arr2    = CheckTestObjectNewArray(stackAllocator, 10);
}

TEST_F(StackAllocatorTest, NewDeleteMixed)
{
    TestObject* arr1    = CheckTestObjectNewArray(stackAllocator, 10);
    TestObject* object1 = CheckTestObjectNew(stackAllocator, 1, 2.1f, 'a', false, 10.6f);
    TestObject* object2 = CheckTestObjectNew(stackAllocator, 1, 2.1f, 'a', false, 10.6f);
    TestObject* arr2    = CheckTestObjectNewArray(stackAllocator, 10);

    stackAllocator.DeleteArray(arr2);
    stackAllocator.Delete(object2);
    stackAllocator.Delete(object1);
    stackAllocator.DeleteArray(arr1);
}

TEST_F(StackAllocatorTest, Reset)
{
    StackAllocator<> stackAllocator2 = StackAllocator<>(10 * (sizeof(TestObject) + std::max(alignof(TestObject), std::size_t(1))), nullptr);
    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckTestObjectNew(stackAllocator2, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }

    stackAllocator2.Reset();

    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckTestObjectNew(stackAllocator2, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }
}

TEST_F(StackAllocatorTest, GetUsedSizeNew)
{
    const int numObjects = 10;
    for (size_t i = 0; i < numObjects; i++)
    {
        TestObject* object = CheckTestObjectNew(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }

    EXPECT_EQ(stackAllocator.GetUsedSize(), numObjects * (sizeof(TestObject) + std::max(alignof(TestObject), size_t(1))));
}

TEST_F(StackAllocatorTest, GetUsedSizeNewDelete)
{
    const int                numObjects = 10;
    std::vector<TestObject*> objects;
    for (size_t i = 0; i < numObjects; i++)
    {
        TestObject* object = CheckTestObjectNew(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
        objects.push_back(object);
    }

    for (int i = numObjects - 1; i >= 0; i--)
    {
        stackAllocator.Delete(objects[i]);
    }

    EXPECT_EQ(stackAllocator.GetUsedSize(), 0);
}

TEST_F(StackAllocatorTest, GetUsedSizeNewArray)
{
    const int numObjects = 10;

    TestObject* arr = CheckTestObjectNewArray(stackAllocator, numObjects);

    EXPECT_EQ(stackAllocator.GetUsedSize(), std::max(alignof(TestObject), size_t(8) + numObjects * sizeof(TestObject)));
}

TEST_F(StackAllocatorTest, GetUsedSizeNewDeleteArray)
{
    const int   numObjects = 10;
    TestObject* arr        = CheckTestObjectNewArray(stackAllocator, numObjects);

    stackAllocator.DeleteArray(arr);

    EXPECT_EQ(stackAllocator.GetUsedSize(), 0);
}

#ifdef MEMARENA_ENABLE_ASSERTS

class StackAllocatorDeathTest : public ::testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}

    StackAllocator<> stackAllocator = StackAllocator<>(10_MB);
};

TEST_F(StackAllocatorDeathTest, MaxSizeAllocation)
{
    // TODO Write proper exit messages
    ASSERT_DEATH({ StackAllocator<> stackAllocator2 = StackAllocator<>(std::numeric_limits<Offset>::max() + 1); }, ".*");
}

TEST_F(StackAllocatorDeathTest, NewOutOfMemory)
{
    constexpr StackAllocatorPolicy allocatorPolicy = StackAllocatorPolicy(SizeCheckPolicy::Check);

    StackAllocator<allocatorPolicy> stackAllocator2 = StackAllocator<allocatorPolicy>(10);

    // TODO Write proper exit messages
    ASSERT_DEATH({ TestObject* object = stackAllocator2.NewRaw<TestObject>(1, 2.1f, 'a', false, 10.6f); }, ".*");
}

TEST_F(StackAllocatorDeathTest, DeleteNullPointer)
{
    constexpr StackAllocatorPolicy allocatorPolicy = StackAllocatorPolicy(NullCheckPolicy::Check);

    StackAllocator<allocatorPolicy> stackAllocator2 = StackAllocator<allocatorPolicy>(1_KB);

    int* nullPointer = nullptr;

    // TODO Write proper exit messages
    ASSERT_DEATH({ stackAllocator2.Delete(nullPointer); }, ".*");
}

TEST_F(StackAllocatorDeathTest, DeleteNotOwnedPointer)
{
    constexpr StackAllocatorPolicy allocatorPolicy = StackAllocatorPolicy(OwnershipCheckPolicy::Check);

    StackAllocator<allocatorPolicy> stackAllocator2 = StackAllocator<allocatorPolicy>(1_KB);

    int* pointer = new int(10);

    // TODO Write proper exit messages
    ASSERT_DEATH({ stackAllocator2.Delete(pointer); }, ".*");
}

TEST_F(StackAllocatorDeathTest, MemoryStompingDetection)
{
    constexpr StackAllocatorPolicy allocatorPolicy = StackAllocatorPolicy(BoundsCheckPolicy::Basic);

    StackAllocator<allocatorPolicy> stackAllocator2 = StackAllocator<allocatorPolicy>(1_KB);

    TestObject* testObject = stackAllocator2.NewRaw<TestObject>(1, 2.1f, 'a', false, 10.6f);

    EXPECT_EQ(testObject->a, 1);
    EXPECT_EQ(testObject->b, 2.1f);
    EXPECT_EQ(testObject->c, 'a');
    EXPECT_EQ(testObject->d, false);
    EXPECT_EQ(testObject->e, 10.6f);

    TestObject* testObject2 = stackAllocator2.NewRaw<TestObject>(1, 2.1f, 'a', false, 10.6f);

    EXPECT_EQ(testObject2->a, 1);
    EXPECT_EQ(testObject2->b, 2.1f);
    EXPECT_EQ(testObject2->c, 'a');
    EXPECT_EQ(testObject2->d, false);
    EXPECT_EQ(testObject2->e, 10.6f);

    stackAllocator2.Delete(testObject);

    TestObject2* testObject3 = stackAllocator2.NewRaw<TestObject2>(1, 2.1, 3.4, false, std::vector<int>());

    // TODO Write proper exit messages
    stackAllocator2.Delete(testObject3);

    ASSERT_DEATH({ stackAllocator2.Delete(testObject2); }, ".*");
}

TEST_F(StackAllocatorDeathTest, DeleteWrongOrder)
{
    constexpr StackAllocatorPolicy allocatorPolicy = StackAllocatorPolicy(StackCheckPolicy::Check);

    StackAllocator<allocatorPolicy> stackAllocator2 = StackAllocator<allocatorPolicy>(1_KB);

    TestObject* testObject  = stackAllocator2.NewRaw<TestObject>(1, 2.1f, 'a', false, 10.6f);
    TestObject* testObject2 = stackAllocator2.NewRaw<TestObject>(1, 2.1f, 'a', false, 10.6f);

    // TODO Write proper exit messages
    ASSERT_DEATH({ stackAllocator2.Delete(testObject); }, ".*");
}

#endif
