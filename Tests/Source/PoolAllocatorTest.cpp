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
Object* CheckNew(PoolAllocator<>& allocator, Args&&... argList)
{
    Object* object = allocator.New<Object>(std::forward<Args>(argList)...);

    EXPECT_EQ(*object, Object(std::forward<Args>(argList)...));

    return object;
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
    TestObject* object = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);

    poolAllocator.Delete(object);
}

TEST_F(PoolAllocatorTest, NewDeleteMultipleObjects)
{
    std::vector<TestObject*> objects1;

    for (int i = 0; i < 10; i++)
    {
        TestObject* object =
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
    TestObject* object = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);

    poolAllocator.Delete(object);

    TestObject* object2 = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);
}

TEST_F(PoolAllocatorTest, NewDeleteNewMultipleObjects)
{
    for (int i = 0; i < 10; i++)
    {
        TestObject* object =
            CheckNew<TestObject>(poolAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);

        poolAllocator.Delete(object);
    }
}

TEST_F(PoolAllocatorTest, GetUsedSizeNew)
{
    const int numObjects = 10;
    for (size_t i = 0; i < numObjects; i++)
    {
        TestObject* object = CheckNew<TestObject>(poolAllocator, i, 1.5F, 'a' + i, i % 2, 2.5F);
    }

    EXPECT_EQ(poolAllocator.GetUsedSize(), numObjects * (sizeof(TestObject)));
}

template <PoolAllocatorPolicy policy>
void ThreadFunction(PoolAllocator<policy>& poolAllocator)
{
    std::vector<TestObject*> objects;

    for (size_t i = 0; i < 10000; i++)
    {
        TestObject* testObject = poolAllocator.template New<TestObject>(1, 1.5F, 'a', false, 2.5F);
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

// TEST_F(PoolAllocatorTest, Templated)
// {
//     MallocatorTemplated<TestObject> mallocatorTemplated{};

//     TestObject* testObject = mallocatorTemplated.New(1, 1.5F, 'a', false, 2.5F);
//     EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));

//     MallocArrayPtr<TestObject> testObject3 = mallocatorTemplated.NewArray(10, 1, 1.5F, 'a', false, 2.5F);
//     EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));

//     mallocatorTemplated.Delete(testObject);
//     mallocatorTemplated.DeleteArray(testObject3);

//     EXPECT_EQ(mallocatorTemplated.GetUsedSize(), 0);
// }

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

// #ifdef MEMARENA_ENABLE_ASSERTS

// class MallocatorDeathTest : public ::testing::Test
// {
//   protected:
//     void SetUp() override {}
//     void TearDown() override {}

//     PoolAllocator<> poolAllocator = PoolAllocator();
// };

// TEST_F(MallocatorDeathTest, DeleteNullptrPointer)
// {
//     constexpr PoolAllocatorPolicy policy = PoolAllocatorPolicy::NullDeallocCheck;
//     PoolAllocator<policy>      poolAllocator2{};
//     MallocPtr<int>             ptr{nullptr, 0};

//     // TODO Write proper exit messages
//     ASSERT_DEATH({ poolAllocator2.Delete(ptr); }, ".*");
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

// #endif
