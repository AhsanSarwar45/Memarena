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
    void SetUp() override { MemoryTracker::ResetAllocators(); }
    void TearDown() override {}
};

#define DECLARE_CHECK_HELPERS(currentPolicy)                                                                                     \
    constexpr PoolAllocatorSettings currentPolicy##settings = {.policy = PoolAllocatorPolicy::currentPolicy};                    \
    template <typename Object, typename... Args>                                                                                 \
    PoolPtr<Object> CheckNew(PoolAllocator<currentPolicy##settings>& allocator, Args&&... argList)                               \
    {                                                                                                                            \
        PoolPtr<Object> object = allocator.New<Object>(std::forward<Args>(argList)...);                                          \
                                                                                                                                 \
        EXPECT_EQ(*object, Object(std::forward<Args>(argList)...));                                                              \
                                                                                                                                 \
        return object;                                                                                                           \
    }                                                                                                                            \
                                                                                                                                 \
    template <typename Object, typename... Args>                                                                                 \
    PoolArrayPtr<Object> CheckNewArray(PoolAllocator<currentPolicy##settings>& allocator, size_t objectCount, Args&&... argList) \
    {                                                                                                                            \
        PoolArrayPtr<Object> arr = allocator.NewArray<Object>(objectCount, std::forward<Args>(argList)...);                      \
                                                                                                                                 \
        for (int i = 0; i < objectCount; i++)                                                                                    \
        {                                                                                                                        \
            EXPECT_EQ(arr[i], Object(std::forward<Args>(argList)...));                                                           \
        }                                                                                                                        \
        return arr;                                                                                                              \
    }                                                                                                                            \
    template <typename Object, typename... Args>                                                                                 \
    Object* CheckNewRaw(PoolAllocator<currentPolicy##settings>& allocator, Args&&... argList)                                    \
    {                                                                                                                            \
        Object* object = allocator.NewRaw<Object>(std::forward<Args>(argList)...);                                               \
                                                                                                                                 \
        EXPECT_EQ(*object, Object(std::forward<Args>(argList)...));                                                              \
                                                                                                                                 \
        return object;                                                                                                           \
    }

DECLARE_CHECK_HELPERS(Default)
DECLARE_CHECK_HELPERS(Debug)
DECLARE_CHECK_HELPERS(Release)

#define POLICY_TEST(name, currentPolicy, code)                                                                           \
    TEST_F(PoolAllocatorTest, name##_##currentPolicy##Policy)                                                            \
    {                                                                                                                    \
        constexpr PoolAllocatorSettings        currentPolicy##settings = {.policy = PoolAllocatorPolicy::currentPolicy}; \
        PoolAllocator<currentPolicy##settings> poolAllocator{sizeof(TestObject), 1000};                                  \
        code                                                                                                             \
    }

#define ALLOCATOR_TEST(name, code)    \
    POLICY_TEST(name, Default, code); \
    POLICY_TEST(name, Debug, code);   \
    POLICY_TEST(name, Release, code);

#define ALLOCATOR_DEBUG_TEST(name, code) \
    POLICY_TEST(name, Default, code);    \
    POLICY_TEST(name, Debug, code);

ALLOCATOR_TEST(Initialize, { EXPECT_EQ(poolAllocator.GetUsedSize(), 0); })

ALLOCATOR_TEST(RawNewSingleObject, { CheckNewRaw<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F); })

ALLOCATOR_TEST(RawNewMultipleObjects, {
    for (int i = 0; i < 10; i++)
    {
        CheckNewRaw<TestObject>(poolAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);
    }
})

ALLOCATOR_TEST(RawNewDeleteSingleObject, {
    TestObject* object = CheckNewRaw<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);

    poolAllocator.DeleteRaw(object);
})

ALLOCATOR_TEST(RawNewDeleteMultipleObjects, {
    std::vector<TestObject*> objects1;

    for (int i = 0; i < 10; i++)
    {
        TestObject* object =
            CheckNewRaw<TestObject>(poolAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);

        objects1.push_back(object);
    }

    for (int i = static_cast<int>(objects1.size()) - 1; i >= 0; i--)
    {
        poolAllocator.DeleteRaw(objects1[i]);
    }
})

ALLOCATOR_TEST(RawNewDeleteNewSingleObject, {
    TestObject* object = CheckNewRaw<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);

    poolAllocator.DeleteRaw(object);

    TestObject* object2 = CheckNewRaw<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);
})

ALLOCATOR_TEST(RawNewDeleteNewMultipleObjects, {
    for (int i = 0; i < 10; i++)
    {
        TestObject* object =
            CheckNewRaw<TestObject>(poolAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);

        poolAllocator.DeleteRaw(object);
    }
})

ALLOCATOR_TEST(NewSingleObject, { CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F); })

ALLOCATOR_TEST(NewMultipleObjects, {
    for (int i = 0; i < 10; i++)
    {
        CheckNew<TestObject>(poolAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);
    }
})

ALLOCATOR_TEST(NewDeleteSingleObject, {
    PoolPtr<TestObject> object = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);

    poolAllocator.Delete(object);
})

ALLOCATOR_TEST(NewDeleteMultipleObjects, {
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
})

ALLOCATOR_TEST(NewDeleteNewSingleObject, {
    PoolPtr<TestObject> object = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);

    poolAllocator.Delete(object);

    PoolPtr<TestObject> object2 = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);
})

ALLOCATOR_TEST(NewDeleteNewMultipleObjects, {
    for (int i = 0; i < 10; i++)
    {
        PoolPtr<TestObject> object =
            CheckNew<TestObject>(poolAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);

        poolAllocator.Delete(object);
    }
})

ALLOCATOR_TEST(NewArray, { PoolArrayPtr<TestObject> arr = CheckNewArray<TestObject>(poolAllocator, 10, 1, 2.1F, 'a', false, 10.6F); })

ALLOCATOR_TEST(NewDeleteArray, {
    PoolArrayPtr<TestObject> arr = CheckNewArray<TestObject>(poolAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
    poolAllocator.DeleteArray(arr);
})

ALLOCATOR_TEST(NewMixed, {
    PoolArrayPtr<TestObject> arr1    = CheckNewArray<TestObject>(poolAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
    PoolPtr<TestObject>      object1 = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);
    PoolPtr<TestObject>      object2 = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);
    PoolArrayPtr<TestObject> arr2    = CheckNewArray<TestObject>(poolAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
})

ALLOCATOR_TEST(NewDeleteMixed, {
    PoolArrayPtr<TestObject> arr1    = CheckNewArray<TestObject>(poolAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
    PoolPtr<TestObject>      object1 = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);
    PoolPtr<TestObject>      object2 = CheckNew<TestObject>(poolAllocator, 1, 2.1F, 'a', false, 10.6F);
    PoolArrayPtr<TestObject> arr2    = CheckNewArray<TestObject>(poolAllocator, 10, 1, 2.1F, 'a', false, 10.6F);

    poolAllocator.DeleteArray(arr2);
    poolAllocator.Delete(object2);
    poolAllocator.Delete(object1);
    poolAllocator.DeleteArray(arr1);
})

template <PoolAllocatorSettings settings>
void ThreadFunction(PoolAllocator<settings>& poolAllocator)
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
    constexpr PoolAllocatorSettings settings = {.policy = PoolAllocatorPolicy::Default | PoolAllocatorPolicy::Multithreaded};

    PoolAllocator<settings> poolAllocator{sizeof(TestObject), 50000};

    std::thread thread1(&ThreadFunction<settings>, std::ref(poolAllocator));
    std::thread thread2(&ThreadFunction<settings>, std::ref(poolAllocator));
    std::thread thread3(&ThreadFunction<settings>, std::ref(poolAllocator));
    std::thread thread4(&ThreadFunction<settings>, std::ref(poolAllocator));

    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();

    EXPECT_EQ(poolAllocator.GetUsedSize(), sizeof(TestObject) * 4 * 10000);
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
//     PoolAllocatorPMR poolAllocatorPMR{sizeof(TestObject), 1000};

//     auto vec = std::pmr::vector<TestObject>(0, &poolAllocatorPMR);

//     const int numIters = 10;

//     for (int i = 0; i < numIters; i++)
//     {
//         vec.emplace_back(1, 1.5F, 'a', false, 2.5F);
//     }

//     for (int i = 0; i < numIters; i++)
//     {
//         EXPECT_EQ(vec[i], TestObject(1, 1.5F, 'a', false, 2.5F));
//     }

//     EXPECT_EQ(poolAllocatorPMR.GetInternalAllocator().GetUsedSize(), 256);
//     EXPECT_EQ(vec.size(), numIters);
// }

TEST_F(PoolAllocatorTest, MemoryTracker)
{
    constexpr PoolAllocatorSettings settings = {.policy = PoolAllocatorPolicy::Debug};

    PoolAllocator<settings> poolAllocator{sizeof(Int64), 1000};

    Int64* num = static_cast<Int64*>(poolAllocator.Allocate("Testing/PoolAllocator").GetPtr());

    const AllocatorVector allocators = MemoryTracker::GetAllocators();

    EXPECT_EQ(allocators.size(), 1);
    if (allocators.size() > 0)
    {
        EXPECT_EQ(allocators[0]->totalSize, sizeof(Int64) * 1000);
        EXPECT_EQ(allocators[0]->usedSize, sizeof(Int64));
        EXPECT_EQ(allocators[0]->allocationCount, 1);
        EXPECT_EQ(allocators[0]->deallocationCount, 0);
        EXPECT_EQ(allocators[0]->allocations[0].category, std::string("Testing/PoolAllocator"));
        EXPECT_EQ(allocators[0]->allocations[0].size, sizeof(Int64));
    }
}

ALLOCATOR_DEBUG_TEST(GetUsedSizeNew, {
    const int numObjects = 10;
    for (size_t i = 0; i < numObjects; i++)
    {
        PoolPtr<TestObject> object = poolAllocator.New<TestObject>(i, 1.5F, 'a' + i, i % 2, 2.5F);
    }

    EXPECT_EQ(poolAllocator.GetUsedSize(), numObjects * (sizeof(TestObject)));
})

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
//     PoolAllocator<policy>         poolAllocator{sizeof(TestObject), 1000};
//     PoolPtr<TestObject>           ptr = PoolPtr<TestObject>(nullptr);

//     // TODO Write proper exit messages
//     ASSERT_DEATH({ poolAllocator.Delete(ptr); }, ".*");
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
//     PoolAllocator<policy>      poolAllocator{};

//     auto ptr = poolAllocator.Allocate(4);

//     poolAllocator.Deallocate(ptr);
//     // TODO Write proper exit messages
//     ASSERT_DEATH({ poolAllocator.Deallocate(ptr); }, ".*");
// }

#endif
