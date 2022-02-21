#include <gtest/gtest.h>

#ifdef MEMORY_MANAGER_DEBUG
    #define MEMORY_MANAGER_ENABLE_ASSERTS
    #define MEMORY_MANAGER_DEBUG_BREAK
#endif

#include <MemoryManager/StackAllocator.hpp>

#include "MemoryTestObjects.hpp"

using namespace Memory;

class StackAllocatorTest : public ::testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}

    StackAllocator stackAllocator = StackAllocator(10_MB);
};

TestObject* CheckTestObjectNew(StackAllocator& stackAllocator, int a, float b, char c, bool d, float e)
{
    TestObject* object = stackAllocator.New<TestObject>(a, b, c, d, e);

    EXPECT_EQ(object->a, a);
    EXPECT_EQ(object->b, b);
    EXPECT_EQ(object->c, c);
    EXPECT_EQ(object->d, d);
    EXPECT_EQ(object->e, e);

    return object;
}

TestObject2* CheckTestObjectNew2(StackAllocator& stackAllocator, int a, double b, double c, bool d, std::vector<int> e)
{
    TestObject2* object = stackAllocator.New<TestObject2>(a, b, c, d, e);

    EXPECT_EQ(object->a, a);
    EXPECT_EQ(object->b, b);
    EXPECT_EQ(object->c, c);
    EXPECT_EQ(object->d, d);
    EXPECT_EQ(object->e.size(), e.size());

    return object;
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

TEST_F(StackAllocatorTest, NewThenDeleteSingleObject)
{
    TestObject* object = CheckTestObjectNew(stackAllocator, 1, 2.1f, 'a', false, 10.6f);

    stackAllocator.Delete(object);
}

TEST_F(StackAllocatorTest, NewThenDeleteMultipleSameObjects)
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

TEST_F(StackAllocatorTest, NewThenDeleteMultipleDifferentObjects)
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

TEST_F(StackAllocatorTest, NewThenDeleteThenNewSingleObject)
{
    TestObject* object = CheckTestObjectNew(stackAllocator, 1, 2.1f, 'a', false, 10.6f);

    stackAllocator.Delete(object);

    TestObject* object2 = CheckTestObjectNew(stackAllocator, 1, 2.1f, 'a', false, 10.6f);
}

TEST_F(StackAllocatorTest, NewThenDeleteThenNewMultipleSameObjects)
{
    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckTestObjectNew(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);

        stackAllocator.Delete(object);
    }
}

TEST_F(StackAllocatorTest, NewThenDeleteThenNewMultipleDifferentObjects)
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

TEST_F(StackAllocatorTest, Clear)
{
    StackAllocator stackAllocator2 = StackAllocator(10 * (sizeof(TestObject) + 8), nullptr, 8);
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

#ifdef MEMORY_MANAGER_ENABLE_ASSERTS

TEST(StackAllocatorDeathTest, NewOutOfMemory)
{
    StackAllocator stackAllocator2 = StackAllocator(10);

    ASSERT_DEATH({ TestObject* object = stackAllocator2.New<TestObject>(1, 2.1f, 'a', false, 10.6f); },
                 "Error: The allocator StackAllocator is out of memory!");
}

#endif

// TEST_F(StackAllocatorTest, NullPtrDeallocate)
// {

//     TestObject* object  = stackAllocator.New<TestObject>(1, 2.1f, 'a', false, 10.6f);
//     TestObject* object2 = stackAllocator.New<TestObject>(1, 2.1f, 'a', false, 10.6f);
//     TestObject* object3 = stackAllocator.New<TestObject>(1, 2.1f, 'a', false, 10.6f);
// }
