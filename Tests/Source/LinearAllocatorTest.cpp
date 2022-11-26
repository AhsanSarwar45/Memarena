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

class LinearAllocatorTest : public ::testing::Test
{
  protected:
    void SetUp() override { MemoryTracker::ResetAllocators(); }
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

TEST_F(LinearAllocatorTest, RawNewSingleObject) { CheckNewRaw<TestObject>(linearAllocator, 1, 2.1F, 'a', false, 10.6f); }

TEST_F(LinearAllocatorTest, RawNewMultipleObjects)
{
    for (size_t i = 0; i < 10; i++)
    {
        CheckNewRaw<TestObject>(linearAllocator, i, 1.5F, 'a' + i, i % 2, 2.5F);
    }
    for (size_t i = 0; i < 10; i++)
    {
        CheckNewRaw<TestObject2>(linearAllocator, i, 1.5, 2.5, i % 2, Pair{1, 2.5F});
    }
}

TEST_F(LinearAllocatorTest, RawNewMixed)
{
    TestObject* arr1    = CheckNewArrayRaw<TestObject>(linearAllocator, 10, 1, 2.1F, 'a', false, 10.6f);
    TestObject* object1 = CheckNewRaw<TestObject>(linearAllocator, 1, 2.1F, 'a', false, 10.6f);
    TestObject* object2 = CheckNewRaw<TestObject>(linearAllocator, 1, 2.1F, 'a', false, 10.6f);
    TestObject* arr2    = CheckNewArrayRaw<TestObject>(linearAllocator, 10, 1, 2.1F, 'a', false, 10.6f);
}

TEST_F(LinearAllocatorTest, Release)
{
    LinearAllocator<> linearAllocator2 = LinearAllocator<>(10 * sizeof(TestObject));
    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckNewRaw<TestObject>(linearAllocator2, i, 1.5F, 'a' + i, i % 2, 2.5F);
    }

    linearAllocator2.Release();

    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckNewRaw<TestObject>(linearAllocator2, i, 1.5F, 'a' + i, i % 2, 2.5F);
    }
}

template <LinearAllocatorPolicy policy>
void ThreadFunction(LinearAllocator<policy>& linearAllocator)
{
    std::vector<TestObject*> objects;

    for (size_t i = 0; i < 10000; i++)
    {
        TestObject* testObject = linearAllocator.template NewRaw<TestObject>(1, 1.5F, 'a', false, 2.5F);
        EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));

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

TEST_F(LinearAllocatorTest, Growable)
{
    constexpr LinearAllocatorPolicy policy = LinearAllocatorPolicy::Default | LinearAllocatorPolicy::Growable;

    LinearAllocator<policy> linearAllocator2{sizeof(TestObject) * 2};

    const int numObjects = 10;
    for (size_t i = 0; i < numObjects; i++)
    {
        TestObject* testObject = linearAllocator2.NewRaw<TestObject>(1, 1.5F, 'a', false, 2.5F);
        EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));
    }

    EXPECT_EQ(linearAllocator2.GetUsedSize(), sizeof(TestObject) * 10);

    const int blockSize = sizeof(TestObject) * 2 - 4;

    LinearAllocator<policy> linearAllocator3{blockSize};

    for (size_t i = 0; i < numObjects; i++)
    {
        TestObject* testObject = linearAllocator3.NewRaw<TestObject>(1, 1.5F, 'a', false, 2.5F);
        EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));
    }

    EXPECT_EQ(linearAllocator3.GetUsedSize(), blockSize * 9 + static_cast<int>(sizeof(TestObject)));
    EXPECT_EQ(linearAllocator3.GetTotalSize(), blockSize * 10);
}

TEST_F(LinearAllocatorTest, Templated)
{
    LinearAllocatorTemplated<TestObject> linearAllocatorTemplated{10_KB};

    TestObject* testObject = linearAllocatorTemplated.NewRaw(1, 1.5F, 'a', false, 2.5F);
    EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));

    TestObject* testObject3 = linearAllocatorTemplated.NewArrayRaw(10, 1, 1.5F, 'a', false, 2.5F);
    EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));

    linearAllocatorTemplated.Release();

    EXPECT_EQ(linearAllocatorTemplated.GetUsedSize(), 0);
}

TEST_F(LinearAllocatorTest, PmrVector)
{
    const int blockSize = 10_KB;

    LinearAllocatorPMR<LinearAllocatorPolicy::Debug> linearAllocatorPMR{blockSize};

    auto vec = std::pmr::vector<TestObject>(0, &linearAllocatorPMR);

    const int numIters = 10;

    for (int i = 0; i < numIters; i++)
    {
        vec.emplace_back(1, 1.5F, 'a', false, 2.5F);
    }

    for (int i = 0; i < numIters; i++)
    {
        EXPECT_EQ(vec[i], TestObject(1, 1.5F, 'a', false, 2.5F));
    }

    EXPECT_EQ(linearAllocatorPMR.GetInternalAllocator().GetUsedSize(), 496);

    EXPECT_EQ(vec.size(), numIters);
}

TEST_F(LinearAllocatorTest, MemoryTracker)
{
    LinearAllocator<LinearAllocatorPolicy::Debug> linearAllocator2{1_MB};

    const AllocatorVector allocators = MemoryTracker::GetAllocators();
    EXPECT_EQ(allocators.size(), 1);

    int* num = static_cast<int*>(linearAllocator2.Allocate<int>("Testing/LinearAllocator"));

    if (allocators.size() > 0)
    {
        EXPECT_EQ(allocators[0]->totalSize, 1_MB);
        EXPECT_EQ(allocators[0]->usedSize, sizeof(int));
        EXPECT_EQ(allocators[0]->allocationCount, 1);
        EXPECT_EQ(allocators[0]->deallocationCount, 0);
        EXPECT_EQ(allocators[0]->allocations[0].category, std::string("Testing/LinearAllocator"));
        EXPECT_EQ(allocators[0]->allocations[0].size, sizeof(int));
    }
}

TEST_F(LinearAllocatorTest, DefaultBaseAllocator)
{
    LinearAllocator<LinearAllocatorPolicy::Debug> linearAllocator2{1_MB};

    int* num = static_cast<int*>(linearAllocator2.Allocate<int>("Testing/LinearAllocator"));

    EXPECT_EQ(Allocator::GetDefaultAllocator()->GetTotalSize(), 11_MB);
}

TEST_F(LinearAllocatorTest, CustomBaseAllocator)
{
    auto baseAllocator = std::make_shared<Mallocator<MallocatorPolicy::Default>>("Mallocator");

    LinearAllocator<LinearAllocatorPolicy::Debug> linearAllocator2{1_MB, "TestAllocator", baseAllocator};

    int* num = static_cast<int*>(linearAllocator2.Allocate<int>("Testing/LinearAllocator"));

    EXPECT_EQ(baseAllocator->GetTotalSize(), 1_MB);
}

TEST_F(LinearAllocatorTest, GetUsedSizeNew)
{
    LinearAllocator<LinearAllocatorPolicy::Debug> linearAllocator2{1_MB};

    const int numObjects = 10;
    for (size_t i = 0; i < numObjects; i++)
    {
        TestObject* object = linearAllocator2.NewRaw<TestObject>(i, 1.5F, 'a' + i, i % 2, 2.5F);
    }

    EXPECT_EQ(linearAllocator2.GetUsedSize(), numObjects * (sizeof(TestObject)));
}

TEST_F(LinearAllocatorTest, GetUsedSizeNewArray)
{
    LinearAllocator<LinearAllocatorPolicy::Debug> linearAllocator2{1_MB};

    const int   numObjects = 10;
    TestObject* arr        = linearAllocator2.NewArrayRaw<TestObject>(numObjects, 1, 2.1F, 'a', false, 10.6f);

    EXPECT_EQ(linearAllocator2.GetUsedSize(), std::max(alignof(TestObject), numObjects * sizeof(TestObject)));
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
    ASSERT_DEATH({ TestObject* object = linearAllocator2.NewRaw<TestObject>(1, 2.1F, 'a', false, 10.6f); }, ".*");
}

#endif
