#include <gtest/gtest.h>

#include <thread>

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
        CheckNewRaw<TestObject2>(linearAllocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2.5f});
    }
}

TEST_F(LinearAllocatorTest, RawNewMixed)
{
    TestObject* arr1    = CheckNewArrayRaw<TestObject>(linearAllocator, 10, 1, 2.1f, 'a', false, 10.6f);
    TestObject* object1 = CheckNewRaw<TestObject>(linearAllocator, 1, 2.1f, 'a', false, 10.6f);
    TestObject* object2 = CheckNewRaw<TestObject>(linearAllocator, 1, 2.1f, 'a', false, 10.6f);
    TestObject* arr2    = CheckNewArrayRaw<TestObject>(linearAllocator, 10, 1, 2.1f, 'a', false, 10.6f);
}

TEST_F(LinearAllocatorTest, Release)
{
    LinearAllocator<> linearAllocator2 = LinearAllocator<>(10 * sizeof(TestObject));
    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckNewRaw<TestObject>(linearAllocator2, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }

    linearAllocator2.Release();

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

template <LinearAllocatorPolicy policy>
void ThreadFunction(LinearAllocator<policy>& linearAllocator)
{
    std::vector<TestObject*> objects;

    for (size_t i = 0; i < 10000; i++)
    {
        TestObject* testObject = linearAllocator.template NewRaw<TestObject>(1, 1.5f, 'a', false, 2.5f);
        EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5f));

        objects.push_back(testObject);
    }
}

TEST_F(LinearAllocatorTest, Multithreaded)
{
    constexpr LinearAllocatorPolicy policy = LinearAllocatorPolicy::Default | LinearAllocatorPolicy::Multithreaded;

    LinearAllocator<policy> linearAllocator2{sizeof(TestObject) * 5 * 10000};

    std::thread thread1(&ThreadFunction<policy>, std::ref(linearAllocator2));
    std::thread thread2(&ThreadFunction<policy>, std::ref(linearAllocator2));
    std::thread thread3(&ThreadFunction<policy>, std::ref(linearAllocator2));
    std::thread thread4(&ThreadFunction<policy>, std::ref(linearAllocator2));

    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();

    EXPECT_EQ(linearAllocator2.GetUsedSize(), sizeof(TestObject) * 4 * 10000);
}

TEST_F(LinearAllocatorTest, Resizable)
{
    constexpr LinearAllocatorPolicy policy = LinearAllocatorPolicy::Default | LinearAllocatorPolicy::Resizable;

    LinearAllocator<policy> linearAllocator2{sizeof(TestObject) * 2};

    const int numObjects = 10;
    for (size_t i = 0; i < numObjects; i++)
    {
        TestObject* testObject = linearAllocator2.NewRaw<TestObject>(1, 1.5f, 'a', false, 2.5f);
        EXPECT_EQ(*testObject, TestObject(1, 1.5f, 'a', false, 2.5f));
    }

    EXPECT_EQ(linearAllocator2.GetUsedSize(), (sizeof(TestObject) * 10));
}

TEST_F(LinearAllocatorTest, Templated)
{
    LinearAllocatorTemplated<TestObject> linearAllocatorTemplated{10_KB};

    TestObject* testObject = linearAllocatorTemplated.NewRaw(1, 1.5f, 'a', false, 2.5f);
    EXPECT_EQ(*testObject, TestObject(1, 1.5f, 'a', false, 2.5f));

    TestObject* testObject3 = linearAllocatorTemplated.NewArrayRaw(10, 1, 1.5f, 'a', false, 2.5f);
    EXPECT_EQ(*testObject, TestObject(1, 1.5f, 'a', false, 2.5f));

    linearAllocatorTemplated.Release();

    EXPECT_EQ(linearAllocatorTemplated.GetUsedSize(), 0);
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
    ASSERT_DEATH({ LinearAllocator linearAllocator2{Size(std::numeric_limits<Offset>::max()) + 1}; }, ".*");
}

TEST_F(LinearAllocatorDeathTest, NewOutOfMemory)
{
    LinearAllocator linearAllocator2{10};

    // TODO Write proper exit messages
    ASSERT_DEATH({ TestObject* object = linearAllocator2.NewRaw<TestObject>(1, 2.1f, 'a', false, 10.6f); }, ".*");
}

#endif
