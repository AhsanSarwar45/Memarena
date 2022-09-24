#include <gtest/gtest.h>

#include <iostream>
#include <memory>
#include <memory_resource>
#include <thread>

#include <Memarena/Memarena.hpp>
#include <vector>

#include "Macro.hpp"
#include "MemoryTestObjects.hpp"
#include "Source/MemoryTracker.hpp"
#include "Source/Policies/Policies.hpp"

using namespace Memarena;
using namespace Memarena::SizeLiterals;

class PoolAllocatorTest : public ::testing::Test
{
  protected:
    void SetUp() override { MemoryTracker::ResetBaseAllocators(); }
    void TearDown() override {}

    PoolAllocator<> poolAllocator = PoolAllocator(sizeof(TestObject), 1000);
};

template <typename Object, typename... Args>
PoolPtr<Object> CheckNew(PoolAllocator<>& allocator, Args&&... argList)
{
    PoolPtr<Object> object = allocator.New<Object>(std::forward<Args>(argList)...);

    EXPECT_EQ(*object, Object(std::forward<Args>(argList)...));

    return object;
}

template <typename Object, typename... Args>
PoolArrayPtr<Object> CheckNewArray(PoolAllocator<>& allocator, size_t objectCount, Args&&... argList)
{
    PoolArrayPtr<Object> arr = allocator.NewArray<Object>(objectCount, std::forward<Args>(argList)...);

    for (int i = 0; i < objectCount; i++)
    {
        EXPECT_EQ(arr[i], Object(std::forward<Args>(argList)...));
    }
    return arr;
}

TEST_F(PoolAllocatorTest, Initialize) { EXPECT_EQ(poolAllocator.GetUsedSize(), 0); }
TEST_F(PoolAllocatorTest, NewSingleObject) { CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F); }

TEST_F(PoolAllocatorTest, NewMultipleObjects)
{
    for (int i = 0; i < 10; i++)
    {
        CheckNew<TestObject>(poolAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);
    }
}

TEST_F(PoolAllocatorTest, NewDeleteSingleObject)
{
    PoolPtr<TestObject> object = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);

    poolAllocator.Delete(object);
}

TEST_F(PoolAllocatorTest, NewDeleteMultipleObjects)
{
    std::vector<PoolPtr<TestObject>> objects1;

    for (int i = 0; i < 10; i++)
    {
        PoolPtr<TestObject> object =
            CheckNew<TestObject>(poolAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);

        objects1.push_back(object);
    }

    for (int i = static_cast<int>(objects1.size()) - 1; i >= 0; i--)
    {
        poolAllocator.Delete(objects1[i]);
    }
}

TEST_F(PoolAllocatorTest, NewDeleteNewSingleObject)
{
    PoolPtr<TestObject> object = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);

    poolAllocator.Delete(object);

    PoolPtr<TestObject> object2 = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);
}

TEST_F(PoolAllocatorTest, NewDeleteNewMultipleObjects)
{
    for (int i = 0; i < 10; i++)
    {
        PoolPtr<TestObject> object =
            CheckNew<TestObject>(poolAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);

        poolAllocator.Delete(object);
    }
}

TEST_F(PoolAllocatorTest, GetUsedSizeNew)
{
    const int numObjects = 10;
    for (size_t i = 0; i < numObjects; i++)
    {
        PoolPtr<TestObject> object = CheckNew<TestObject>(poolAllocator, i, 1.5F, 'a' + i, i % 2, 2.5F);
    }

    EXPECT_EQ(poolAllocator.GetUsedSize(), numObjects * (sizeof(TestObject)));
}

TEST_F(PoolAllocatorTest, NewArray)
{
    PoolArrayPtr<TestObject> arr = CheckNewArray<TestObject>(poolAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
}

TEST_F(PoolAllocatorTest, NewDeleteArray)
{
    PoolArrayPtr<TestObject> arr = CheckNewArray<TestObject>(poolAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
    poolAllocator.DeleteArray(arr);
}

TEST_F(PoolAllocatorTest, NewMixed)
{
    PoolArrayPtr<TestObject> arr1    = CheckNewArray<TestObject>(poolAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
    PoolPtr<TestObject>      object1 = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);
    PoolPtr<TestObject>      object2 = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);
    PoolArrayPtr<TestObject> arr2    = CheckNewArray<TestObject>(poolAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
}

TEST_F(PoolAllocatorTest, NewDeleteMixed)
{
    PoolArrayPtr<TestObject> arr1    = CheckNewArray<TestObject>(poolAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
    PoolPtr<TestObject>      object1 = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);
    PoolPtr<TestObject>      object2 = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);
    PoolArrayPtr<TestObject> arr2    = CheckNewArray<TestObject>(poolAllocator, 10, 1, 2.1F, 'a', false, 10.6F);

    poolAllocator.DeleteArray(arr2);
    poolAllocator.Delete(object2);
    poolAllocator.Delete(object1);
    poolAllocator.DeleteArray(arr1);
}

template <PoolAllocatorPolicy policy>
void ThreadFunction(PoolAllocator<policy>& poolAllocator)
{
    std::vector<PoolPtr<TestObject>> objects;

    for (size_t i = 0; i < 10000; i++)
    {
        PoolPtr<TestObject> testObject = poolAllocator.template New<TestObject>(1, 1.5F, 'a', false, 2.5F);
        EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));

        objects.push_back(testObject);
    }

    // for (auto object : objects)
    // {
    //     poolAllocator.Delete(object);
    // }
}

TEST_F(PoolAllocatorTest, Multithreaded)
{
    constexpr PoolAllocatorPolicy policy = PoolAllocatorPolicy::Default | PoolAllocatorPolicy::Multithreaded;

    PoolAllocator<policy> poolAllocator2{sizeof(TestObject), 50000};

    std::thread thread1(&ThreadFunction<policy>, std::ref(poolAllocator2));
    std::thread thread2(&ThreadFunction<policy>, std::ref(poolAllocator2));
    std::thread thread3(&ThreadFunction<policy>, std::ref(poolAllocator2));
    std::thread thread4(&ThreadFunction<policy>, std::ref(poolAllocator2));

    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();

    EXPECT_EQ(poolAllocator2.GetUsedSize(), sizeof(TestObject) * 4 * 10000);
}

TEST_F(PoolAllocatorTest, Templated)
{
    PoolAllocatorTemplated<TestObject> poolAllocatorTemplated{10};

    PoolPtr<TestObject> testObject = poolAllocatorTemplated.New(1, 1.5F, 'a', false, 2.5F);
    EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));

    poolAllocatorTemplated.Delete(testObject);
    EXPECT_EQ(poolAllocatorTemplated.GetUsedSize(), 0);
}

// TEST_F(PoolAllocatorTest, PmrVector)
// {
//     MallocatorPMR mallocatorPMR{};

//     auto vec = std::pmr::vector<TestObject>(0, &mallocatorPMR);

//     const int numIters = 10;

//     for (int i = 0; i < numIters; i++)
//     {
//         vec.emplace_back(1, 1.5F, 'a', false, 2.5F);
//     }

//     for (int i = 0; i < numIters; i++)
//     {
//         EXPECT_EQ(vec[i], TestObject(1, 1.5F, 'a', false, 2.5F));
//     }

//     EXPECT_EQ(mallocatorPMR.GetInternalAllocator().GetUsedSize(), 256);
//     EXPECT_EQ(vec.size(), numIters);
// }

// TEST_F(PoolAllocatorTest, MemoryTracker)
// {
//     constexpr PoolAllocatorPolicy policy = PoolAllocatorPolicy::Debug;
//     PoolAllocator<policy>      poolAllocator2{};

//     int* num = static_cast<int*>(poolAllocator2.Allocate<int>("Testing/PoolAllocator").GetPtr());

//     const AllocatorVector allocators = MemoryTracker::GetBaseAllocators();

//     EXPECT_EQ(allocators.size(), 1);
//     if (allocators.size() > 0)
//     {
//         EXPECT_EQ(allocators[0]->totalSize, sizeof(int));
//         EXPECT_EQ(allocators[0]->usedSize, sizeof(int));
//         EXPECT_EQ(allocators[0]->allocationCount, 1);
//         EXPECT_EQ(allocators[0]->deallocationCount, 0);
//         EXPECT_EQ(allocators[0]->allocations[0].category, std::string("Testing/PoolAllocator"));
//         EXPECT_EQ(allocators[0]->allocations[0].size, sizeof(int));
//     }
//     EXPECT_EQ(MemoryTracker::GetTotalAllocatedSize(), sizeof(int));
// }

#ifdef MEMARENA_ENABLE_ASSERTS

class PoolAllocatorDeathTest : public ::testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}

    PoolAllocator<> poolAllocator = PoolAllocator(sizeof(TestObject), 1000);
};

// TEST_F(PoolAllocatorDeathTest, DeleteNullPointer)
// {
//     constexpr PoolAllocatorPolicy policy = PoolAllocatorPolicy::NullDeallocCheck;
//     PoolAllocator<policy>         poolAllocator2{sizeof(TestObject), 1000};
//     PoolPtr<TestObject>           ptr = PoolPtr<TestObject>(nullptr);

//     // TODO Write proper exit messages
//     ASSERT_DEATH({ poolAllocator2.Delete(ptr); }, ".*");
// }

TEST_F(PoolAllocatorDeathTest, NewOutOfMemory)
{
    PoolAllocator poolAllocator2{sizeof(TestObject), 1};

    PoolPtr<TestObject> object = poolAllocator2.New<TestObject>(1, 2.1F, 'a', false, 10.6F);

    // TODO Write proper exit messages
    ASSERT_DEATH({ PoolPtr<TestObject> object = poolAllocator2.New<TestObject>(1, 2.1F, 'a', false, 10.6F); }, ".*");
}

// TEST_F(PoolAllocatorDeathTest, DeleteNotOwnedPointer)
// {
//     TestObject* pointer = new TestObject(1, 2.1F, 'a', false, 10.6F);

//     // TODO Write proper exit messages
//     ASSERT_DEATH({ poolAllocator.Delete(pointer); }, ".*");
// }

TEST_F(PoolAllocatorDeathTest, NewWrongSizedObject)
{
    // TODO Write proper exit messages
    ASSERT_DEATH({ PoolPtr<TestObject2> pointer = poolAllocator.New<TestObject2>(1, 1.5F, 'a', false, Pair{1, 2.5}); }, ".*");
}

// TEST_F(PoolAllocatorDeathTest, DeleteWrongSizedObject)
// {
//     PoolPtr<TestObject2> pointer = new TestObject2(1, 1.5F, 'a', false, Pair{1, 2.5});

//     // TODO Write proper exit messages
//     ASSERT_DEATH({ poolAllocator.Delete(pointer); }, ".*");
// }

// TEST_F(MallocatorDeathTest, DoubleFree)
// {
//     constexpr PoolAllocatorPolicy policy = PoolAllocatorPolicy::DoubleFreePrevention;
//     PoolAllocator<policy>      poolAllocator2{};

//     auto ptr = poolAllocator2.Allocate(4);

//     poolAllocator2.Deallocate(ptr);
//     // TODO Write proper exit messages
//     ASSERT_DEATH({ poolAllocator2.Deallocate(ptr); }, ".*");
// }

#endif
