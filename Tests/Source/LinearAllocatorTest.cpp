#include <gtest/gtest.h>

#include <memory>
#include <memory_resource>
#include <thread>

#include <Memarena/Memarena.hpp>
#include <vector>

#include "Macro.hpp"
#include "MemoryTestObjects.hpp"
#include "Source/Allocators/LinearAllocator/LinearAllocator.hpp"
#include "Source/MemoryTracker.hpp"
#include "Source/Policies/Policies.hpp"

using namespace Memarena;
using namespace Memarena::SizeLiterals;

class LinearAllocatorTest : public ::testing::Test
{
  protected:
    void SetUp() override { MemoryTracker::ResetAllocators(); }
    void TearDown() override {}
};

#define DECLARE_CHECK_HELPERS(currentPolicy)                                                                             \
    constexpr LinearAllocatorSettings currentPolicy##settings = {.policy = LinearAllocatorPolicy::currentPolicy};        \
    template <typename Object, typename... Args>                                                                         \
    Object* CheckNewRaw(LinearAllocator<currentPolicy##settings>& allocator, Args&&... argList)                          \
    {                                                                                                                    \
        Object* object = allocator.NewRaw<Object>(std::forward<Args>(argList)...);                                       \
        EXPECT_EQ(*object, Object(std::forward<Args>(argList)...));                                                      \
        return object;                                                                                                   \
    }                                                                                                                    \
    template <typename Object, typename... Args>                                                                         \
    Object* CheckNewArrayRaw(LinearAllocator<currentPolicy##settings>& allocator, size_t objectCount, Args&&... argList) \
    {                                                                                                                    \
        Object* arr = allocator.NewArrayRaw<Object>(objectCount, std::forward<Args>(argList)...);                        \
        for (size_t i = 0; i < objectCount; i++)                                                                         \
        {                                                                                                                \
            EXPECT_EQ(arr[i], Object(std::forward<Args>(argList)...));                                                   \
        }                                                                                                                \
        return arr;                                                                                                      \
    }

DECLARE_CHECK_HELPERS(Default)
DECLARE_CHECK_HELPERS(Debug)
DECLARE_CHECK_HELPERS(Release)

#define POLICY_TEST(name, currentPolicy, code)                                                                               \
    TEST_F(LinearAllocatorTest, name##_##currentPolicy##Policy)                                                              \
    {                                                                                                                        \
        constexpr LinearAllocatorSettings        currentPolicy##settings = {.policy = LinearAllocatorPolicy::currentPolicy}; \
        LinearAllocator<currentPolicy##settings> linearAllocator{1_MB};                                                      \
        code                                                                                                                 \
    }

#define ALLOCATOR_TEST(name, code)    \
    POLICY_TEST(name, Default, code); \
    POLICY_TEST(name, Debug, code);   \
    POLICY_TEST(name, Release, code);

#define ALLOCATOR_DEBUG_TEST(name, code) \
    POLICY_TEST(name, Default, code);    \
    POLICY_TEST(name, Debug, code);

ALLOCATOR_TEST(Initialize, { EXPECT_EQ(linearAllocator.GetUsedSize(), 0); })

ALLOCATOR_TEST(RawNewSingleObject, { CheckNewRaw<TestObject>(linearAllocator, 1, 2.1F, 'a', false, 10.6f); })

ALLOCATOR_TEST(RawNewMultipleObjects, {
    for (size_t i = 0; i < 10; i++)
    {
        CheckNewRaw<TestObject>(linearAllocator, i, 1.5F, 'a' + i, i % 2, 2.5F);
    }
    for (size_t i = 0; i < 10; i++)
    {
        CheckNewRaw<TestObject2>(linearAllocator, i, 1.5, 2.5, i % 2, Pair{1, 2.5F});
    }
})

ALLOCATOR_TEST(RawNewMixed, {
    TestObject* arr1    = CheckNewArrayRaw<TestObject>(linearAllocator, 10, 1, 2.1F, 'a', false, 10.6f);
    TestObject* object1 = CheckNewRaw<TestObject>(linearAllocator, 1, 2.1F, 'a', false, 10.6f);
    TestObject* object2 = CheckNewRaw<TestObject>(linearAllocator, 1, 2.1F, 'a', false, 10.6f);
    TestObject* arr2    = CheckNewArrayRaw<TestObject>(linearAllocator, 10, 1, 2.1F, 'a', false, 10.6f);
})

TEST_F(LinearAllocatorTest, Release)
{
    LinearAllocator<> linearAllocator = LinearAllocator<>(10 * sizeof(TestObject));
    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckNewRaw<TestObject>(linearAllocator, i, 1.5F, 'a' + i, i % 2, 2.5F);
    }

    linearAllocator.Release();

    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckNewRaw<TestObject>(linearAllocator, i, 1.5F, 'a' + i, i % 2, 2.5F);
    }
}

template <LinearAllocatorSettings settings>
void ThreadFunction(LinearAllocator<settings>& linearAllocator)
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
    constexpr LinearAllocatorSettings settings = {.policy = LinearAllocatorPolicy::Default | LinearAllocatorPolicy::Multithreaded};

    LinearAllocator<settings> linearAllocator{sizeof(TestObject) * 5 * 10000};

    std::thread thread1(&ThreadFunction<settings>, std::ref(linearAllocator));
    std::thread thread2(&ThreadFunction<settings>, std::ref(linearAllocator));
    std::thread thread3(&ThreadFunction<settings>, std::ref(linearAllocator));
    std::thread thread4(&ThreadFunction<settings>, std::ref(linearAllocator));

    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();

    EXPECT_EQ(linearAllocator.GetUsedSize(), sizeof(TestObject) * 4 * 10000);
}

TEST_F(LinearAllocatorTest, Growable)
{
    constexpr LinearAllocatorSettings settings = {.policy = LinearAllocatorPolicy::Default | LinearAllocatorPolicy::Growable};

    LinearAllocator<settings> linearAllocator{sizeof(TestObject) * 2};

    const int numObjects = 10;
    for (size_t i = 0; i < numObjects; i++)
    {
        TestObject* testObject = linearAllocator.NewRaw<TestObject>(1, 1.5F, 'a', false, 2.5F);
        EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));
    }

    EXPECT_EQ(linearAllocator.GetUsedSize(), sizeof(TestObject) * 10);

    const int blockSize = sizeof(TestObject) * 2 - 4;

    LinearAllocator<settings> linearAllocator2{blockSize};

    for (size_t i = 0; i < numObjects; i++)
    {
        TestObject* testObject = linearAllocator2.NewRaw<TestObject>(1, 1.5F, 'a', false, 2.5F);
        EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));
    }

    EXPECT_EQ(linearAllocator2.GetUsedSize(), blockSize * 9 + static_cast<int>(sizeof(TestObject)));
    EXPECT_EQ(linearAllocator2.GetTotalSize(), blockSize * 10);
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
    const int                         blockSize = 10_KB;
    constexpr LinearAllocatorSettings settings  = {.policy = LinearAllocatorPolicy::Debug};
    LinearAllocatorPMR<settings>      linearAllocatorPMR{blockSize};

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
};

TEST_F(LinearAllocatorTest, MemoryTracker)
{
    constexpr LinearAllocatorSettings settings = {.policy = LinearAllocatorPolicy::Debug};
    LinearAllocator<settings>         linearAllocator{1_MB};

    const AllocatorVector allocators = MemoryTracker::GetAllocators();
    EXPECT_EQ(allocators.size(), 1);

    int* num = static_cast<int*>(linearAllocator.Allocate<int>("Testing/LinearAllocator"));

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

ALLOCATOR_DEBUG_TEST(DefaultBaseAllocator, {
    int* num = static_cast<int*>(linearAllocator.Allocate<int>("Testing/LinearAllocator"));
    EXPECT_EQ(Allocator::GetDefaultAllocator()->GetTotalSize(), 1_MB);
})

TEST_F(LinearAllocatorTest, CustomBaseAllocator)
{
    constexpr MallocatorSettings mallocatorSettings = {.policy = MallocatorPolicy::Default};

    auto baseAllocator = std::make_shared<Mallocator<mallocatorSettings>>("Mallocator");

    constexpr LinearAllocatorSettings settings = {.policy = LinearAllocatorPolicy::Debug};
    LinearAllocator<settings>         linearAllocator{1_MB, "TestAllocator", baseAllocator};

    int* num = static_cast<int*>(linearAllocator.Allocate<int>("Testing/LinearAllocator"));
    EXPECT_EQ(baseAllocator->GetTotalSize(), 1_MB);
}

ALLOCATOR_DEBUG_TEST(GetUsedSizeNew, {
    const int numObjects = 10;
    for (size_t i = 0; i < numObjects; i++)
    {
        TestObject* object = linearAllocator.NewRaw<TestObject>(i, 1.5F, 'a' + i, i % 2, 2.5F);
    }
    EXPECT_EQ(linearAllocator.GetUsedSize(), numObjects * (sizeof(TestObject)));
})

ALLOCATOR_DEBUG_TEST(GetUsedSizeNewArray, {
    const int   numObjects = 10;
    TestObject* arr        = linearAllocator.NewArrayRaw<TestObject>(numObjects, 1, 2.1F, 'a', false, 10.6f);
    EXPECT_EQ(linearAllocator.GetUsedSize(), std::max(alignof(TestObject), numObjects * sizeof(TestObject)));
})

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
    ASSERT_DEATH({ LinearAllocator linearAllocator{Size(std::numeric_limits<Offset>::max()) + 1}; }, ".*");
}

TEST_F(LinearAllocatorDeathTest, NewOutOfMemory)
{
    LinearAllocator linearAllocator2{10};

    // TODO Write proper exit messages
    ASSERT_DEATH({ TestObject* object = linearAllocator2.NewRaw<TestObject>(1, 2.1F, 'a', false, 10.6f); }, ".*");
}

#endif
