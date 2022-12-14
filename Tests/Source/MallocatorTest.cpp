#include <gtest/gtest.h>

#include <memory>
#include <memory_resource>
#include <thread>

#include <Memarena/Memarena.hpp>
#include <vector>

#include "Macro.hpp"
#include "MemoryTestObjects.hpp"
#include "Source/Allocators/Mallocator/Mallocator.hpp"
#include "Source/MemoryTracker.hpp"
#include "Source/Policies/Policies.hpp"

using namespace Memarena;
using namespace Memarena::SizeLiterals;

class MallocatorTest : public ::testing::Test
{
  protected:
    void SetUp() override { MemoryTracker::ResetBaseAllocators(); }
    void TearDown() override {}
};

#define DECLARE_CHECK_HELPERS(currentPolicy)                                                                                              \
    constexpr MallocatorSettings currentPolicy##settings = {.policy = MallocatorPolicy::currentPolicy};                                   \
    template <typename Object, typename... Args>                                                                                          \
    Object* CheckNewRaw(Mallocator<currentPolicy##settings>& allocator, Args&&... argList)                                                \
    {                                                                                                                                     \
        Object* object = allocator.NewRaw<Object>(std::forward<Args>(argList)...);                                                        \
                                                                                                                                          \
        EXPECT_EQ(*object, Object(std::forward<Args>(argList)...));                                                                       \
                                                                                                                                          \
        return object;                                                                                                                    \
    }                                                                                                                                     \
    template <typename Object, typename... Args>                                                                                          \
    Object* CheckNewArrayRaw(Mallocator<currentPolicy##settings>& allocator, size_t objectCount, Args&&... argList)                       \
    {                                                                                                                                     \
        Object* arr = allocator.NewArrayRaw<Object>(objectCount, std::forward<Args>(argList)...);                                         \
                                                                                                                                          \
        for (int i = 0; i < objectCount; i++)                                                                                             \
        {                                                                                                                                 \
            EXPECT_EQ(arr[i], Object(std::forward<Args>(argList)...));                                                                    \
        }                                                                                                                                 \
        return arr;                                                                                                                       \
    }                                                                                                                                     \
                                                                                                                                          \
    template <typename Object, typename... Args>                                                                                          \
    Memarena::MallocPtr<Object> CheckNew(Mallocator<currentPolicy##settings>& allocator, Args&&... argList)                               \
    {                                                                                                                                     \
        Memarena::MallocPtr<Object> object = allocator.New<Object>(std::forward<Args>(argList)...);                                       \
                                                                                                                                          \
        EXPECT_EQ(*(object.GetPtr()), Object(std::forward<Args>(argList)...));                                                            \
                                                                                                                                          \
        return object;                                                                                                                    \
    }                                                                                                                                     \
                                                                                                                                          \
    template <typename Object, typename... Args>                                                                                          \
    Memarena::MallocArrayPtr<Object> CheckNewArray(Mallocator<currentPolicy##settings>& allocator, size_t objectCount, Args&&... argList) \
    {                                                                                                                                     \
        Memarena::MallocArrayPtr<Object> arr = allocator.NewArray<Object>(objectCount, std::forward<Args>(argList)...);                   \
                                                                                                                                          \
        for (int i = 0; i < objectCount; i++)                                                                                             \
        {                                                                                                                                 \
            EXPECT_EQ(arr[i], Object(std::forward<Args>(argList)...));                                                                    \
        }                                                                                                                                 \
        return arr;                                                                                                                       \
    }

DECLARE_CHECK_HELPERS(Default)
DECLARE_CHECK_HELPERS(Debug)
DECLARE_CHECK_HELPERS(Release)

#define POLICY_TEST(name, currentPolicy, code)                                                                     \
    TEST_F(MallocatorTest, name##_##currentPolicy##Policy)                                                         \
    {                                                                                                              \
        constexpr MallocatorSettings        currentPolicy##settings = {.policy = MallocatorPolicy::currentPolicy}; \
        Mallocator<currentPolicy##settings> mallocator{};                                                          \
        code                                                                                                       \
    }

#define ALLOCATOR_TEST(name, code)    \
    POLICY_TEST(name, Default, code); \
    POLICY_TEST(name, Debug, code);   \
    POLICY_TEST(name, Release, code);

#define ALLOCATOR_DEBUG_TEST(name, code) \
    POLICY_TEST(name, Default, code);    \
    POLICY_TEST(name, Debug, code);

ALLOCATOR_TEST(Initialize, { EXPECT_EQ(mallocator.GetUsedSize(), 0); })

ALLOCATOR_TEST(NewSingleObject, { CheckNew<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F); })

ALLOCATOR_TEST(NewMultipleObjects, {
    for (int i = 0; i < 10; i++)
    {
        CheckNew<TestObject>(mallocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);
    }
    for (int i = 0; i < 10; i++)
    {
        CheckNew<TestObject2>(mallocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2.5});
    }
})

ALLOCATOR_TEST(NewDeleteSingleObject, {
    MallocPtr<TestObject> object = CheckNew<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);

    mallocator.Delete(object);
})

ALLOCATOR_TEST(NewDeleteMultipleObjects, {
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
})

ALLOCATOR_TEST(NewDeleteNewSingleObject, {
    MallocPtr<TestObject> object = CheckNew<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);

    mallocator.Delete(object);

    MallocPtr<TestObject> object2 = CheckNew<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);
})

ALLOCATOR_TEST(NewDeleteNewMultipleObjects, {
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
})

ALLOCATOR_TEST(NewArray, { MallocArrayPtr<TestObject> arr = CheckNewArray<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F); })

ALLOCATOR_TEST(NewDeleteArray, {
    MallocArrayPtr<TestObject> arr = CheckNewArray<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F);
    mallocator.DeleteArray(arr);
})

ALLOCATOR_TEST(NewMixed, {
    MallocArrayPtr<TestObject> arr1    = CheckNewArray<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F);
    MallocPtr<TestObject>      object1 = CheckNew<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);
    MallocPtr<TestObject>      object2 = CheckNew<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);
    MallocArrayPtr<TestObject> arr2    = CheckNewArray<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F);
})

ALLOCATOR_TEST(NewDeleteMixed, {
    MallocArrayPtr<TestObject> arr1    = CheckNewArray<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F);
    MallocPtr<TestObject>      object1 = CheckNew<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);
    MallocPtr<TestObject>      object2 = CheckNew<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);
    MallocArrayPtr<TestObject> arr2    = CheckNewArray<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F);

    mallocator.DeleteArray(arr2);
    mallocator.Delete(object2);
    mallocator.Delete(object1);
    mallocator.DeleteArray(arr1);
})

ALLOCATOR_TEST(RawNewSingleObject, { CheckNewRaw<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F); })

ALLOCATOR_TEST(RawNewMultipleObjects, {
    for (int i = 0; i < 10; i++)
    {
        CheckNewRaw<TestObject>(mallocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);
    }
    for (int i = 0; i < 10; i++)
    {
        CheckNewRaw<TestObject2>(mallocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2.5});
    }
})

ALLOCATOR_TEST(RawNewDeleteSingleObject, {
    TestObject* object = CheckNewRaw<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);

    mallocator.Delete(object);
})

ALLOCATOR_TEST(RawNewDeleteMultipleObjects, {
    std::vector<TestObject*>  objects1;
    std::vector<TestObject2*> objects2;

    for (int i = 0; i < 10; i++)
    {
        TestObject* object =
            CheckNewRaw<TestObject>(mallocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);

        objects1.push_back(object);
    }
    for (int i = 0; i < 10; i++)
    {
        TestObject2* object = CheckNewRaw<TestObject2>(mallocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2});

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
})

ALLOCATOR_TEST(RawNewDeleteNewSingleObject, {
    TestObject* object = CheckNewRaw<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);

    mallocator.Delete(object);

    TestObject* object2 = CheckNewRaw<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);
})

ALLOCATOR_TEST(RawNewDeleteNewMultipleObjects, {
    for (int i = 0; i < 10; i++)
    {
        TestObject* object =
            CheckNewRaw<TestObject>(mallocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);

        mallocator.Delete(object);
    }
    for (int i = 0; i < 10; i++)
    {
        TestObject2* object = CheckNewRaw<TestObject2>(mallocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2.5});

        mallocator.Delete(object);
    }
})

ALLOCATOR_TEST(RawNewArray, { TestObject* arr = CheckNewArrayRaw<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F); })

ALLOCATOR_TEST(RawNewDeleteArray, {
    TestObject* arr = CheckNewArrayRaw<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F);
    mallocator.DeleteArray(arr);
})

ALLOCATOR_TEST(RawNewMixed, {
    TestObject* arr1    = CheckNewArrayRaw<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F);
    TestObject* object1 = CheckNewRaw<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);
    TestObject* object2 = CheckNewRaw<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);
    TestObject* arr2    = CheckNewArrayRaw<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F);
})

ALLOCATOR_TEST(RawNewDeleteMixed, {
    TestObject* arr1    = CheckNewArrayRaw<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F);
    TestObject* object1 = CheckNewRaw<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);
    TestObject* object2 = CheckNewRaw<TestObject>(mallocator, 1, 2.1F, 'a', false, 10.6F);
    TestObject* arr2    = CheckNewArrayRaw<TestObject>(mallocator, 10, 1, 2.1F, 'a', false, 10.6F);

    mallocator.DeleteArray(arr2);
    mallocator.Delete(object2);
    mallocator.Delete(object1);
    mallocator.DeleteArray(arr1);
})

TEST_F(MallocatorTest, GetUsedSizeNew)
{
    constexpr MallocatorSettings settings = {.policy = MallocatorPolicy::Debug};
    Mallocator<settings>         mallocator2{};

    const int numObjects = 10;
    for (size_t i = 0; i < numObjects; i++)
    {
        MallocPtr<TestObject> object = mallocator2.New<TestObject>(i, 1.5F, 'a' + i, i % 2, 2.5F);
    }

    EXPECT_EQ(mallocator2.GetUsedSize(), numObjects * (sizeof(TestObject)));
}

TEST_F(MallocatorTest, GetUsedSizeNewArray)
{
    constexpr MallocatorSettings settings = {.policy = MallocatorPolicy::Debug};
    Mallocator<settings>         mallocator2{};

    const int                  numObjects = 10;
    MallocArrayPtr<TestObject> object     = mallocator2.NewArray<TestObject>(numObjects, 1, 2.1F, 'a', false, 10.6f);

    EXPECT_EQ(mallocator2.GetUsedSize(), std::max(alignof(TestObject), numObjects * sizeof(TestObject)));
}

template <MallocatorSettings Settings>
void ThreadFunction(Mallocator<Settings>& mallocator)
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
    constexpr MallocatorSettings settings = {.policy = MallocatorPolicy::Default | MallocatorPolicy::Multithreaded};

    Mallocator<settings> mallocator2{};

    std::thread thread1(&ThreadFunction<settings>, std::ref(mallocator2));
    std::thread thread2(&ThreadFunction<settings>, std::ref(mallocator2));
    std::thread thread3(&ThreadFunction<settings>, std::ref(mallocator2));
    std::thread thread4(&ThreadFunction<settings>, std::ref(mallocator2));

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
    constexpr MallocatorSettings settings = {.policy = MallocatorPolicy::Debug};
    MallocatorPMR<settings>      mallocatorPMR{};

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
    constexpr MallocatorSettings settings = {.policy = MallocatorPolicy::Debug};
    Mallocator<settings>         mallocator2{};

    int* num = static_cast<int*>(mallocator2.Allocate<int>("Testing/Mallocator"));

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
};

TEST_F(MallocatorDeathTest, DeleteNullPointer)
{
    constexpr MallocatorSettings settings = {.policy = MallocatorPolicy::NullDeallocCheck};
    Mallocator<settings>         mallocator{};
    MallocPtr<int>               ptr{nullptr, 0};

    // TODO Write proper exit messages
    ASSERT_DEATH({ mallocator.Delete(ptr); }, ".*");
}

TEST_F(MallocatorDeathTest, DeleteNullPointerRaw)
{
    constexpr MallocatorSettings settings = {.policy = MallocatorPolicy::NullDeallocCheck};
    Mallocator<settings>         mallocator{};
    int*                         ptr = nullptr;

    // TODO Write proper exit messages
    ASSERT_DEATH({ mallocator.Delete(ptr); }, ".*");
}

TEST_F(MallocatorDeathTest, DoubleFree)
{
    constexpr MallocatorSettings settings = {.policy = MallocatorPolicy::DoubleFreePrevention};
    Mallocator<settings>         mallocator{};

    MallocPtr<int> ptr = mallocator.New<int>(4);

    mallocator.Delete(ptr);
    // TODO Write proper exit messages
    ASSERT_DEATH({ mallocator.Delete(ptr); }, ".*");
}

TEST_F(MallocatorDeathTest, DoubleFreeRaw)
{
    constexpr MallocatorSettings settings = {.policy = MallocatorPolicy::DoubleFreePrevention};
    Mallocator<settings>         mallocator{};

    int* ptr = mallocator.NewRaw<int>(4);

    mallocator.Delete(ptr);
    // TODO Write proper exit messages
    ASSERT_DEATH({ mallocator.Delete(ptr); }, ".*");
}

#endif
