#include <gtest/gtest.h>

#include <Memarena/Memarena.hpp>

#include "Macro.hpp"
#include "MemoryTestObjects.hpp"

using namespace Memarena;
using namespace Memarena::SizeOperators;

class StackAllocatorSafeTest : public ::testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}

    StackAllocatorSafe stackAllocatorSafe = StackAllocatorSafe(10_MB);
};

StackPtr<TestObject> CheckTestObjectNew(StackAllocatorSafe& stackAllocatorSafe, int a, float b, char c, bool d, float e)
{
    StackPtr<TestObject> object = stackAllocatorSafe.New<TestObject>(a, b, c, d, e);

    EXPECT_EQ(object->a, a);
    EXPECT_EQ(object->b, b);
    EXPECT_EQ(object->c, c);
    EXPECT_EQ(object->d, d);
    EXPECT_EQ(object->e, e);

    return object;
}

StackPtr<TestObject> CheckTestObjectNewArray(StackAllocatorSafe& stackAllocatorSafe, size_t objectCount)
{
    StackPtr<TestObject> arr = stackAllocatorSafe.NewArray<TestObject>(objectCount, 1, 2.1f, 'a', false, 10.6f);

    for (size_t i = 0; i < objectCount; i++)
    {
        EXPECT_EQ(arr.ptr[i].a, 1);
        EXPECT_EQ(arr.ptr[i].b, 2.1f);
        EXPECT_EQ(arr.ptr[i].c, 'a');
        EXPECT_EQ(arr.ptr[i].d, false);
        EXPECT_EQ(arr.ptr[i].e, 10.6f);
    }

    return arr;
}

StackPtr<TestObject2> CheckTestObjectNew2(StackAllocatorSafe& stackAllocatorSafe, int a, double b, double c, bool d, std::vector<int> e)
{
    StackPtr<TestObject2> object = stackAllocatorSafe.New<TestObject2>(a, b, c, d, e);

    EXPECT_EQ(object->a, a);
    EXPECT_EQ(object->b, b);
    EXPECT_EQ(object->c, c);
    EXPECT_EQ(object->d, d);
    EXPECT_EQ(object->e.size(), e.size());

    return object;
}

TEST_F(StackAllocatorSafeTest, Initialize) { EXPECT_EQ(stackAllocatorSafe.GetUsedSize(), 0); }

TEST_F(StackAllocatorSafeTest, NewSingleObject) { CheckTestObjectNew(stackAllocatorSafe, 1, 2.1f, 'a', false, 10.6f); }

TEST_F(StackAllocatorSafeTest, NewMultipleSameObjects)
{
    for (size_t i = 0; i < 10; i++)
    {
        CheckTestObjectNew(stackAllocatorSafe, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }
}

TEST_F(StackAllocatorSafeTest, NewMultipleDifferentObjects)
{
    for (size_t i = 0; i < 10; i++)
    {
        CheckTestObjectNew(stackAllocatorSafe, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }
    for (size_t i = 0; i < 10; i++)
    {
        CheckTestObjectNew2(stackAllocatorSafe, i, i + 1.5, i + 2.5, i % 2, std::vector<int>(i));
    }
}

TEST_F(StackAllocatorSafeTest, NewThenDeleteSingleObject)
{
    StackPtr<TestObject> object = CheckTestObjectNew(stackAllocatorSafe, 1, 2.1f, 'a', false, 10.6f);

    stackAllocatorSafe.Delete(object);
}

TEST_F(StackAllocatorSafeTest, NewThenDeleteMultipleSameObjects)
{
    std::vector<StackPtr<TestObject>> objects;

    for (size_t i = 0; i < 10; i++)
    {
        StackPtr<TestObject> object = CheckTestObjectNew(stackAllocatorSafe, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);

        objects.push_back(object);
    }

    // Remember to delete in reverse order
    for (int i = objects.size() - 1; i >= 0; i--)
    {
        stackAllocatorSafe.Delete(objects[i]);
    }
}

TEST_F(StackAllocatorSafeTest, NewThenDeleteMultipleDifferentObjects)
{
    std::vector<StackPtr<TestObject>>  objects1;
    std::vector<StackPtr<TestObject2>> objects2;

    for (size_t i = 0; i < 10; i++)
    {
        StackPtr<TestObject> object = CheckTestObjectNew(stackAllocatorSafe, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);

        objects1.push_back(object);
    }
    for (size_t i = 0; i < 10; i++)
    {
        StackPtr<TestObject2> object = CheckTestObjectNew2(stackAllocatorSafe, i, i + 1.5, i + 2.5, i % 2, std::vector<int>(i));

        objects2.push_back(object);
    }

    for (int i = objects2.size() - 1; i >= 0; i--)
    {
        stackAllocatorSafe.Delete(objects2[i]);
    }
    for (int i = objects1.size() - 1; i >= 0; i--)
    {
        stackAllocatorSafe.Delete(objects1[i]);
    }
}

TEST_F(StackAllocatorSafeTest, NewThenDeleteThenNewSingleObject)
{
    StackPtr<TestObject> object = CheckTestObjectNew(stackAllocatorSafe, 1, 2.1f, 'a', false, 10.6f);

    stackAllocatorSafe.Delete(object);

    StackPtr<TestObject> object2 = CheckTestObjectNew(stackAllocatorSafe, 1, 2.1f, 'a', false, 10.6f);
}

TEST_F(StackAllocatorSafeTest, NewThenDeleteThenNewMultipleSameObjects)
{
    for (size_t i = 0; i < 10; i++)
    {
        StackPtr<TestObject> object = CheckTestObjectNew(stackAllocatorSafe, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);

        stackAllocatorSafe.Delete(object);
    }
}

TEST_F(StackAllocatorSafeTest, NewThenDeleteThenNewMultipleDifferentObjects)
{
    for (size_t i = 0; i < 10; i++)
    {
        StackPtr<TestObject> object = CheckTestObjectNew(stackAllocatorSafe, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);

        stackAllocatorSafe.Delete(object);
    }
    for (size_t i = 0; i < 10; i++)
    {
        StackPtr<TestObject2> object = CheckTestObjectNew2(stackAllocatorSafe, i, i + 1.5, i + 2.5, i % 2, std::vector<int>(i));

        stackAllocatorSafe.Delete(object);
    }
}

TEST_F(StackAllocatorSafeTest, NewArray) { StackPtr<TestObject> object = CheckTestObjectNewArray(stackAllocatorSafe, 10); }

TEST_F(StackAllocatorSafeTest, NewThenDeleteArray)
{
    StackPtr<TestObject> object = CheckTestObjectNewArray(stackAllocatorSafe, 10);
    stackAllocatorSafe.DeleteArray(object);
}

TEST_F(StackAllocatorSafeTest, Reset)
{
    StackAllocatorSafe stackAllocator2 =
        StackAllocatorSafe(10 * (sizeof(TestObject) + std::max(alignof(TestObject), std::size_t(1))), nullptr);
    for (size_t i = 0; i < 10; i++)
    {
        StackPtr<TestObject> object = CheckTestObjectNew(stackAllocator2, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }

    stackAllocator2.Reset();

    for (size_t i = 0; i < 10; i++)
    {
        StackPtr<TestObject> object = CheckTestObjectNew(stackAllocator2, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }
}

TEST_F(StackAllocatorSafeTest, GetUsedSizeNew)
{
    const int numObjects = 10;
    for (size_t i = 0; i < numObjects; i++)
    {
        StackPtr<TestObject> object = CheckTestObjectNew(stackAllocatorSafe, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }

    EXPECT_EQ(stackAllocatorSafe.GetUsedSize(), numObjects * sizeof(TestObject));
}

TEST_F(StackAllocatorSafeTest, GetUsedSizeNewDelete)
{
    const int                         numObjects = 10;
    std::vector<StackPtr<TestObject>> objects;
    for (size_t i = 0; i < numObjects; i++)
    {
        StackPtr<TestObject> object = CheckTestObjectNew(stackAllocatorSafe, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
        objects.push_back(object);
    }

    for (int i = numObjects - 1; i >= 0; i--)
    {
        stackAllocatorSafe.Delete(objects[i]);
    }

    EXPECT_EQ(stackAllocatorSafe.GetUsedSize(), 0);
}

TEST_F(StackAllocatorSafeTest, GetUsedSizeNewArray)
{
    const int numObjects = 10;

    StackPtr<TestObject> arr = CheckTestObjectNewArray(stackAllocatorSafe, numObjects);

    EXPECT_EQ(stackAllocatorSafe.GetUsedSize(), numObjects * sizeof(TestObject));
}

TEST_F(StackAllocatorSafeTest, GetUsedSizeNewDeleteArray)
{
    const int            numObjects = 10;
    StackPtr<TestObject> arr        = CheckTestObjectNewArray(stackAllocatorSafe, numObjects);

    stackAllocatorSafe.DeleteArray(arr);

    EXPECT_EQ(stackAllocatorSafe.GetUsedSize(), 0);
}

#ifdef MEMARENA_ENABLE_ASSERTS

class StackAllocatorSafeDeathTest : public ::testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}

    StackAllocatorSafe stackAllocatorSafe = StackAllocatorSafe(10_MB);
};

TEST_F(StackAllocatorSafeDeathTest, MaxSizeAllocation)
{
    // TODO Write proper exit messages
    ASSERT_DEATH({ StackAllocatorSafe stackAllocator2 = StackAllocatorSafe(std::numeric_limits<Offset>::max() + 1); }, ".*");
}

TEST_F(StackAllocatorSafeDeathTest, NewOutOfMemory)
{
    StackAllocatorSafe stackAllocator2 = StackAllocatorSafe(10);

    // TODO Write proper exit messages
    ASSERT_DEATH({ StackPtr<TestObject> object = stackAllocator2.New<TestObject>(1, 2.1f, 'a', false, 10.6f); }, ".*");
}

TEST_F(StackAllocatorSafeDeathTest, DeleteNullPointer)
{
    StackPtr<int> nullPointer = {.ptr = nullptr};

    // TODO Write proper exit messages
    ASSERT_DEATH({ stackAllocatorSafe.Delete(nullPointer); }, ".*");
}

TEST_F(StackAllocatorSafeDeathTest, DeleteNotOwnedPointer)
{
    StackPtr<int> pointer = {.ptr = new int(10)};

    // TODO Write proper exit messages
    ASSERT_DEATH({ stackAllocatorSafe.Delete(pointer); }, ".*");
}

TEST_F(StackAllocatorSafeDeathTest, DeleteWrongOrder)
{
    StackPtr<TestObject> testObject  = stackAllocatorSafe.New<TestObject>(1, 2.1f, 'a', false, 10.6f);
    StackPtr<TestObject> testObject2 = stackAllocatorSafe.New<TestObject>(1, 2.1f, 'a', false, 10.6f);

    // TODO Write proper exit messages
    ASSERT_DEATH({ stackAllocatorSafe.Delete(testObject); }, ".*");
}

#endif
