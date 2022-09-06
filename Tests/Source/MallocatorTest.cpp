#include <gtest/gtest.h>

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

class MallocatorTest : public ::testing::Test
{
  protected:
    void SetUp() override { MemoryTracker::ResetBaseAllocators(); }
    void TearDown() override {}

    Mallocator<> mallocator = Mallocator();
};

template <typename Object, typename... Args>
MallocPtr<Object> CheckNew(Mallocator<>& allocator, Args&&... argList)
{
    MallocPtr<Object> object = allocator.New<Object>(std::forward<Args>(argList)...);

    EXPECT_EQ(*object, Object(std::forward<Args>(argList)...));

    return object;
}
template <typename Object, typename... Args>
MallocArrayPtr<Object> CheckNewArray(Mallocator<>& allocator, size_t objectCount, Args&&... argList)
{
    MallocArrayPtr<Object> arr = allocator.NewArray<Object>(objectCount, std::forward<Args>(argList)...);

    for (size_t i = 0; i < objectCount; i++)
    {
        EXPECT_EQ(arr[i], Object(std::forward<Args>(argList)...));
    }
    return arr;
}

TEST_F(MallocatorTest, Initialize) { EXPECT_EQ(mallocator.GetUsedSize(), 0); }
TEST_F(MallocatorTest, NewSingleObject) { CheckNew<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F); }

TEST_F(MallocatorTest, NewMultipleObjects)
{
    for (int i = 0; i < 10; i++)
    {
        CheckNew<TestObject>(mallocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);
    }
    for (int i = 0; i < 10; i++)
    {
        CheckNew<TestObject2>(mallocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2.5});
    }
}

TEST_F(MallocatorTest, NewDeleteSingleObject)
{
    MallocPtr<TestObject> object = CheckNew<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);

    mallocator.Delete(object);
}

TEST_F(MallocatorTest, NewDeleteMultipleObjects)
{
    std::vector<MallocPtr<TestObject>>  objects1;
    std::vector<MallocPtr<TestObject2>> objects2;

    for (int i = 0; i < 10; i++)
    {
        MallocPtr<TestObject> object =
            CheckNew<TestObject>(mallocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);

        objects1.push_back(object);
    }
    for (int i = 0; i < 10; i++)
    {
        MallocPtr<TestObject2> object = CheckNew<TestObject2>(mallocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2});

        objects2.push_back(object);
    }

    for (int i = static_cast<int>(objects2.size()) - 1; i >= 0; i--)
    {
        mallocator.Delete(objects2[i]);
    }
    for (int i = static_cast<int>(objects1.size()) - 1; i >= 0; i--)
    {
        mallocator.Delete(objects1[i]);
    }
}

TEST_F(MallocatorTest, NewDeleteNewSingleObject)
{
    MallocPtr<TestObject> object = CheckNew<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);

    mallocator.Delete(object);

    MallocPtr<TestObject> object2 = CheckNew<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);
}

TEST_F(MallocatorTest, NewDeleteNewMultipleObjects)
{
    for (int i = 0; i < 10; i++)
    {
        MallocPtr<TestObject> object =
            CheckNew<TestObject>(mallocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);

        mallocator.Delete(object);
    }
    for (int i = 0; i < 10; i++)
    {
        MallocPtr<TestObject2> object = CheckNew<TestObject2>(mallocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2.5});

        mallocator.Delete(object);
    }
}

TEST_F(MallocatorTest, NewArray) { MallocArrayPtr<TestObject> arr = CheckNewArray<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F); }

TEST_F(MallocatorTest, NewDeleteArray)
{
    MallocArrayPtr<TestObject> arr = CheckNewArray<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F);
    mallocator.DeleteArray(arr);
}

TEST_F(MallocatorTest, NewMixed)
{
    MallocArrayPtr<TestObject> arr1    = CheckNewArray<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F);
    MallocPtr<TestObject>      object1 = CheckNew<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);
    MallocPtr<TestObject>      object2 = CheckNew<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);
    MallocArrayPtr<TestObject> arr2    = CheckNewArray<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F);
}

TEST_F(MallocatorTest, NewDeleteMixed)
{
    MallocArrayPtr<TestObject> arr1    = CheckNewArray<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F);
    MallocPtr<TestObject>      object1 = CheckNew<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);
    MallocPtr<TestObject>      object2 = CheckNew<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);
    MallocArrayPtr<TestObject> arr2    = CheckNewArray<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F);

    mallocator.DeleteArray(arr2);
    mallocator.Delete(object2);
    mallocator.Delete(object1);
    mallocator.DeleteArray(arr1);
}

TEST_F(MallocatorTest, GetUsedSizeNew)
{
    const int numObjects = 10;
    for (size_t i = 0; i < numObjects; i++)
    {
        MallocPtr<TestObject> object = CheckNew<TestObject>(mallocator, i, 1.5F, 'a' + i, i % 2, 2.5F);
    }

    EXPECT_EQ(mallocator.GetUsedSize(), numObjects * (sizeof(TestObject)));
}

TEST_F(MallocatorTest, GetUsedSizeNewArray)
{
    const int numObjects = 10;

    MallocArrayPtr<TestObject> arr = CheckNewArray<TestObject>(mallocator, numObjects, 1, 2.1F, 'a', false, 10.6f);

    EXPECT_EQ(mallocator.GetUsedSize(), std::max(alignof(TestObject), numObjects * sizeof(TestObject)));
}

template <MallocatorPolicy policy>
void ThreadFunction(Mallocator<policy>& mallocator)
{
    std::vector<MallocPtr<TestObject>> objects;

    for (size_t i = 0; i < 10000; i++)
    {
        MallocPtr<TestObject> testObject = mallocator.template New<TestObject>(1, 1.5F, 'a', false, 2.5F);
        EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));

        objects.push_back(testObject);
    }

    // for (auto object : objects)
    // {
    //     mallocator.Delete(object);
    // }
}

TEST_F(MallocatorTest, Multithreaded)
{
    constexpr MallocatorPolicy policy = MallocatorPolicy::Default | MallocatorPolicy::Multithreaded;

    Mallocator<policy> mallocator2{};

    std::thread thread1(&ThreadFunction<policy>, std::ref(mallocator2));
    std::thread thread2(&ThreadFunction<policy>, std::ref(mallocator2));
    std::thread thread3(&ThreadFunction<policy>, std::ref(mallocator2));
    std::thread thread4(&ThreadFunction<policy>, std::ref(mallocator2));

    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();

    EXPECT_EQ(mallocator2.GetUsedSize(), sizeof(TestObject) * 4 * 10000);
}

TEST_F(MallocatorTest, Templated)
{
    MallocatorTemplated<TestObject> mallocatorTemplated{};

    MallocPtr<TestObject> testObject = mallocatorTemplated.New(1, 1.5F, 'a', false, 2.5F);
    EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));

    MallocArrayPtr<TestObject> testObject3 = mallocatorTemplated.NewArray(10, 1, 1.5F, 'a', false, 2.5F);
    EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));

    mallocatorTemplated.Delete(testObject);
    mallocatorTemplated.DeleteArray(testObject3);

    EXPECT_EQ(mallocatorTemplated.GetUsedSize(), 0);
}

TEST_F(MallocatorTest, PmrVector)
{
    MallocatorPMR mallocatorPMR{};

    auto vec = std::pmr::vector<TestObject>(0, &mallocatorPMR);

    const int numIters = 10;

    for (int i = 0; i < numIters; i++)
    {
        vec.emplace_back(1, 1.5F, 'a', false, 2.5F);
    }

    for (int i = 0; i < numIters; i++)
    {
        EXPECT_EQ(vec[i], TestObject(1, 1.5F, 'a', false, 2.5F));
    }

    EXPECT_EQ(mallocatorPMR.GetInternalAllocator().GetUsedSize(), 256);
    EXPECT_EQ(vec.size(), numIters);
}

TEST_F(MallocatorTest, MemoryTracker)
{
    constexpr MallocatorPolicy policy = MallocatorPolicy::Debug;
    Mallocator<policy>         mallocator2{};

    int* num = static_cast<int*>(mallocator2.Allocate<int>("Testing/Mallocator").GetPtr());

    const AllocatorVector allocators = MemoryTracker::GetBaseAllocators();

    EXPECT_EQ(allocators.size(), 1);
    if (allocators.size() > 0)
    {
        EXPECT_EQ(allocators[0]->totalSize, sizeof(int));
        EXPECT_EQ(allocators[0]->usedSize, sizeof(int));
        EXPECT_EQ(allocators[0]->allocationCount, 1);
        EXPECT_EQ(allocators[0]->deallocationCount, 0);
        EXPECT_EQ(allocators[0]->allocations[0].category, std::string("Testing/Mallocator"));
        EXPECT_EQ(allocators[0]->allocations[0].size, sizeof(int));
    }
    EXPECT_EQ(MemoryTracker::GetTotalAllocatedSize(), sizeof(int));
}

#ifdef MEMARENA_ENABLE_ASSERTS

class MallocatorDeathTest : public ::testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}

    Mallocator<> mallocator = Mallocator();
};

TEST_F(MallocatorDeathTest, DeleteNullptrPointer)
{
    constexpr MallocatorPolicy policy = MallocatorPolicy::NullDeallocCheck;
    Mallocator<policy>         mallocator2{};
    MallocPtr<int>             ptr{nullptr, 0};

    // TODO Write proper exit messages
    ASSERT_DEATH({ mallocator2.Delete(ptr); }, ".*");
}

TEST_F(MallocatorDeathTest, DoubleFree)
{
    constexpr MallocatorPolicy policy = MallocatorPolicy::DoubleFreePrevention;
    Mallocator<policy>         mallocator2{};

    auto ptr = mallocator2.Allocate(4);

    mallocator2.Deallocate(ptr);
    // TODO Write proper exit messages
    ASSERT_DEATH({ mallocator2.Deallocate(ptr); }, ".*");
}

#endif
