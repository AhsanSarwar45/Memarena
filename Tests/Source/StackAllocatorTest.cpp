#include <functional>
#include <gtest/gtest.h>

#include <thread>

#include <Memarena/Memarena.hpp>

#include "Macro.hpp"
#include "MemoryTestObjects.hpp"
#include "Source/AllocatorData.hpp"
#include "Source/Allocators/StackAllocator/StackAllocator.hpp"
#include "Source/Policies/Policies.hpp"

using namespace Memarena;
using namespace Memarena::SizeLiterals;

constexpr std::array<StackAllocatorPolicy, 2> PoliciesToTest = {StackAllocatorPolicy::Default, StackAllocatorPolicy::Debug};

class StackAllocatorTest : public ::testing::Test
{
  protected:
    void SetUp() override { MemoryTracker::ResetAllocators(); }
    void TearDown() override {}

    // StackAllocator<> stackAllocator{10_MB};
};

#define DECLARE_CHECK_HELPERS(currentPolicy)                                                                              \
    constexpr StackAllocatorSettings currentPolicy##settings = {.policy = StackAllocatorPolicy::currentPolicy};           \
    template <typename Object, typename... Args>                                                                          \
    Object* CheckNewRaw(StackAllocator<currentPolicy##settings>& allocator, Args&&... argList)                            \
    {                                                                                                                     \
        Object* object = allocator.NewRaw<Object>(std::forward<Args>(argList)...);                                        \
                                                                                                                          \
        EXPECT_EQ(*object, Object(std::forward<Args>(argList)...));                                                       \
                                                                                                                          \
        return object;                                                                                                    \
    }                                                                                                                     \
    template <typename Object, typename... Args>                                                                          \
    Object* CheckNewArrayRaw(StackAllocator<currentPolicy##settings>& allocator, size_t objectCount, Args&&... argList)   \
    {                                                                                                                     \
        Object* arr = allocator.NewArrayRaw<Object>(objectCount, std::forward<Args>(argList)...);                         \
                                                                                                                          \
        for (int i = 0; i < objectCount; i++)                                                                             \
        {                                                                                                                 \
            EXPECT_EQ(arr[i], Object(std::forward<Args>(argList)...));                                                    \
        }                                                                                                                 \
        return arr;                                                                                                       \
    }                                                                                                                     \
                                                                                                                          \
    template <typename Object, typename... Args>                                                                          \
    Memarena::StackPtr<Object> CheckNew(StackAllocator<currentPolicy##settings>& allocator, Args&&... argList)            \
    {                                                                                                                     \
        Memarena::StackPtr<Object> object = allocator.New<Object>(std::forward<Args>(argList)...);                        \
                                                                                                                          \
        EXPECT_EQ(*(object.GetPtr()), Object(std::forward<Args>(argList)...));                                            \
                                                                                                                          \
        return object;                                                                                                    \
    }                                                                                                                     \
                                                                                                                          \
    template <typename Object, typename... Args>                                                                          \
    Memarena::StackArrayPtr<Object> CheckNewArray(StackAllocator<currentPolicy##settings>& allocator, size_t objectCount, \
                                                  Args&&... argList)                                                      \
    {                                                                                                                     \
        Memarena::StackArrayPtr<Object> arr = allocator.NewArray<Object>(objectCount, std::forward<Args>(argList)...);    \
                                                                                                                          \
        for (int i = 0; i < objectCount; i++)                                                                             \
        {                                                                                                                 \
            EXPECT_EQ(arr[i], Object(std::forward<Args>(argList)...));                                                    \
        }                                                                                                                 \
        return arr;                                                                                                       \
    }

DECLARE_CHECK_HELPERS(Default)
DECLARE_CHECK_HELPERS(Debug)
DECLARE_CHECK_HELPERS(Release)

#define POLICY_TEST(name, currentPolicy, code)                                                                             \
    TEST_F(StackAllocatorTest, name##_##currentPolicy##Policy)                                                             \
    {                                                                                                                      \
        constexpr StackAllocatorSettings        currentPolicy##settings = {.policy = StackAllocatorPolicy::currentPolicy}; \
        StackAllocator<currentPolicy##settings> stackAllocator{1_MB};                                                      \
        code                                                                                                               \
    }

#define ALLOCATOR_TEST(name, code)    \
    POLICY_TEST(name, Default, code); \
    POLICY_TEST(name, Debug, code);   \
    POLICY_TEST(name, Release, code);

#define ALLOCATOR_DEBUG_TEST(name, code) \
    POLICY_TEST(name, Default, code);    \
    POLICY_TEST(name, Debug, code);

ALLOCATOR_TEST(Initialize, { EXPECT_EQ(stackAllocator.GetUsedSize(), 0); })

ALLOCATOR_TEST(RawNewSingleObject, { CheckNewRaw<TestObject>(stackAllocator, 1, 2.1F, 'a', false, 10.6F); })

ALLOCATOR_TEST(RawNewMultipleObjects, {
    for (int i = 0; i < 10; i++)
    {
        CheckNewRaw<TestObject>(stackAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);
    }
    for (int i = 0; i < 10; i++)
    {
        CheckNewRaw<TestObject2>(stackAllocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2.5});
    }
})

ALLOCATOR_TEST(RawNewDeleteSingleObject, {
    TestObject* object = CheckNewRaw<TestObject>(stackAllocator, 1, 2.1F, 'a', false, 10.6F);
    stackAllocator.Delete(object);
})

ALLOCATOR_TEST(RawNewDeleteMultipleObjects, {
    std::vector<TestObject*>  objects1;
    std::vector<TestObject2*> objects2;

    for (int i = 0; i < 10; i++)
    {
        TestObject* object =
            CheckNewRaw<TestObject>(stackAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);

        objects1.push_back(object);
    }
    for (int i = 0; i < 10; i++)
    {
        TestObject2* object = CheckNewRaw<TestObject2>(stackAllocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2});

        objects2.push_back(object);
    }

    for (int i = static_cast<int>(objects2.size()) - 1; i >= 0; i--)
    {
        stackAllocator.Delete(objects2[i]);
    }
    for (int i = static_cast<int>(objects1.size()) - 1; i >= 0; i--)
    {
        stackAllocator.Delete(objects1[i]);
    }
})

ALLOCATOR_TEST(RawNewDeleteNewSingleObject, {
    TestObject* object = CheckNewRaw<TestObject>(stackAllocator, 1, 2.1F, 'a', false, 10.6F);

    stackAllocator.Delete(object);

    TestObject* object2 = CheckNewRaw<TestObject>(stackAllocator, 1, 2.1F, 'a', false, 10.6F);
})

ALLOCATOR_TEST(RawNewDeleteNewMultipleObjects, {
    for (int i = 0; i < 10; i++)
    {
        TestObject* object =
            CheckNewRaw<TestObject>(stackAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);

        stackAllocator.Delete(object);
    }
    for (int i = 0; i < 10; i++)
    {
        TestObject2* object = CheckNewRaw<TestObject2>(stackAllocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2.5});

        stackAllocator.Delete(object);
    }
})

ALLOCATOR_TEST(RawNewArrayRaw, { TestObject* arr = CheckNewArrayRaw<TestObject>(stackAllocator, 10, 1, 2.1F, 'a', false, 10.6F); })

ALLOCATOR_TEST(RawNewDeleteArray, {
    TestObject* arr = CheckNewArrayRaw<TestObject>(stackAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
    stackAllocator.DeleteArray(arr);
})

ALLOCATOR_TEST(RawNewMixed, {
    TestObject* arr1    = CheckNewArrayRaw<TestObject>(stackAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
    TestObject* object1 = CheckNewRaw<TestObject>(stackAllocator, 1, 2.1F, 'a', false, 10.6F);
    TestObject* object2 = CheckNewRaw<TestObject>(stackAllocator, 1, 2.1F, 'a', false, 10.6F);
    TestObject* arr2    = CheckNewArrayRaw<TestObject>(stackAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
})

ALLOCATOR_TEST(RawNewDeleteMixed, {
    TestObject* arr1    = CheckNewArrayRaw<TestObject>(stackAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
    TestObject* object1 = CheckNewRaw<TestObject>(stackAllocator, 1, 2.1F, 'a', false, 10.6F);
    TestObject* object2 = CheckNewRaw<TestObject>(stackAllocator, 1, 2.1F, 'a', false, 10.6F);
    TestObject* arr2    = CheckNewArrayRaw<TestObject>(stackAllocator, 10, 1, 2.1F, 'a', false, 10.6F);

    stackAllocator.DeleteArray(arr2);
    stackAllocator.Delete(object2);
    stackAllocator.Delete(object1);
    stackAllocator.DeleteArray(arr1);
})

ALLOCATOR_TEST(NewSingleObject, { StackPtr object = CheckNew<TestObject>(stackAllocator, 1, 2.1F, 'a', false, 10.6F); })

ALLOCATOR_TEST(NewMultipleObjects, {
    for (int i = 0; i < 10; i++)
    {
        StackPtr<TestObject> object =
            CheckNew<TestObject>(stackAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);
    }
    for (int i = 0; i < 10; i++)
    {
        StackPtr<TestObject2> object = CheckNew<TestObject2>(stackAllocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2.5});
    }
})

ALLOCATOR_TEST(NewDeleteSingleObject, {
    StackPtr object = CheckNew<TestObject>(stackAllocator, 1, 2.1F, 'a', false, 10.6F);

    stackAllocator.Delete(object);
})
ALLOCATOR_TEST(NewDeleteMultipleObjects, {
    std::vector<StackPtr<TestObject>>  objects1;
    std::vector<StackPtr<TestObject2>> objects2;

    for (int i = 0; i < 10; i++)
    {
        StackPtr<TestObject> object =
            CheckNew<TestObject>(stackAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);

        objects1.push_back(object);
    }
    for (int i = 0; i < 10; i++)
    {
        StackPtr<TestObject2> object = CheckNew<TestObject2>(stackAllocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2.5});

        objects2.push_back(object);
    }

    for (int i = static_cast<int>(objects2.size()) - 1; i >= 0; i--)
    {
        stackAllocator.Delete(objects2[i]);
    }
    for (int i = static_cast<int>(objects1.size()) - 1; i >= 0; i--)
    {
        stackAllocator.Delete(objects1[i]);
    }
})

ALLOCATOR_TEST(NewDeleteNewSingleObject, {
    StackPtr<TestObject> object = CheckNew<TestObject>(stackAllocator, 1, 2.1F, 'a', false, 10.6F);

    stackAllocator.Delete(object);

    StackPtr<TestObject> object2 = CheckNew<TestObject>(stackAllocator, 1, 2.1F, 'a', false, 10.6F);
})

ALLOCATOR_TEST(NewDeleteNewMultipleObjects, {
    for (int i = 0; i < 10; i++)
    {
        StackPtr<TestObject> object =
            CheckNew<TestObject>(stackAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);

        stackAllocator.Delete(object);
    }
    for (int i = 0; i < 10; i++)
    {
        StackPtr<TestObject2> object = CheckNew<TestObject2>(stackAllocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2.5});

        stackAllocator.Delete(object);
    }
})

ALLOCATOR_TEST(NewArray, { StackArrayPtr<TestObject> arr = CheckNewArray<TestObject>(stackAllocator, 10, 1, 2.1F, 'a', false, 10.6F); })

ALLOCATOR_TEST(NewDeleteArray, {
    StackArrayPtr<TestObject> arr = CheckNewArray<TestObject>(stackAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
    stackAllocator.DeleteArray(arr);
})

ALLOCATOR_TEST(NewMixed, {
    StackArrayPtr<TestObject> arr1    = CheckNewArray<TestObject>(stackAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
    StackPtr<TestObject>      object1 = CheckNew<TestObject>(stackAllocator, 1, 2.1F, 'a', false, 10.6F);
    StackPtr<TestObject>      object2 = CheckNew<TestObject>(stackAllocator, 1, 2.1F, 'a', false, 10.6F);
    StackArrayPtr<TestObject> arr2    = CheckNewArray<TestObject>(stackAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
})

ALLOCATOR_TEST(NewDeleteMixed, {
    StackArrayPtr<TestObject> arr1    = CheckNewArray<TestObject>(stackAllocator, 10, 1, 2.1F, 'a', false, 10.6F);
    StackPtr<TestObject>      object1 = CheckNew<TestObject>(stackAllocator, 1, 2.1F, 'a', false, 10.6F);
    StackPtr<TestObject>      object2 = CheckNew<TestObject>(stackAllocator, 1, 2.1F, 'a', false, 10.6F);
    StackArrayPtr<TestObject> arr2    = CheckNewArray<TestObject>(stackAllocator, 10, 1, 2.1F, 'a', false, 10.6F);

    stackAllocator.DeleteArray(arr2);
    stackAllocator.Delete(object2);
    stackAllocator.Delete(object1);
    stackAllocator.DeleteArray(arr1);
})

TEST_F(StackAllocatorTest, Templated)
{
    StackAllocatorTemplated<TestObject> stackAllocatorTemplated{10_KB};

    TestObject* testObject = stackAllocatorTemplated.NewRaw(1, 1.5F, 'a', false, 2.5F);
    EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));
    StackPtr<TestObject> testObject2 = stackAllocatorTemplated.New(1, 1.5F, 'a', false, 2.5F);
    EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));

    TestObject* testObject3 = stackAllocatorTemplated.NewArrayRaw(10, 1, 1.5F, 'a', false, 2.5F);
    EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));
    StackArrayPtr<TestObject> testObject4 = stackAllocatorTemplated.NewArray(10, 1, 1.5F, 'a', false, 2.5F);
    EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));

    stackAllocatorTemplated.DeleteArray(testObject4);
    stackAllocatorTemplated.DeleteArray(testObject3);
    stackAllocatorTemplated.Delete(testObject2);
    stackAllocatorTemplated.Delete(testObject);

    EXPECT_EQ(stackAllocatorTemplated.GetUsedSize(), 0);
}

TEST_F(StackAllocatorTest, PMR)
{
    StackAllocatorPMR stackAllocatorPMR{100};

    auto vec = std::pmr::vector<int>(&stackAllocatorPMR);

    vec.reserve(3);

    vec.push_back(5);
    vec.push_back(8);
    vec.push_back(2);

    EXPECT_EQ(vec[0], 5);
    EXPECT_EQ(vec[1], 8);
    EXPECT_EQ(vec[2], 2);
}

ALLOCATOR_TEST(Owns, {
    StackPtr<TestObject>      object1 = stackAllocator.New<TestObject>(1, 2.1F, 'a', false, 10.6F);
    StackArrayPtr<TestObject> arr1    = stackAllocator.NewArray<TestObject>(10, 1, 2.1F, 'a', false, 10.6F);
    TestObject*               object2 = stackAllocator.NewRaw<TestObject>(1, 2.1F, 'a', false, 10.6F);
    EXPECT_TRUE(stackAllocator.Owns(object1));
    EXPECT_TRUE(stackAllocator.Owns(arr1));
    EXPECT_TRUE(stackAllocator.Owns(object2));
    stackAllocator.Delete(object2);
    stackAllocator.DeleteArray(arr1);
    stackAllocator.Delete(object1);
})

ALLOCATOR_TEST(OwnsNot, {
    int* ptr = new int(1);
    EXPECT_FALSE(stackAllocator.Owns(ptr));
})

template <AllocatorSettings settings>
void ThreadFunction(StackAllocator<settings>& stackAllocator)
{
    std::vector<StackPtr<TestObject>> objects;

    for (int i = 0; i < 10000; i++)
    {
        StackPtr<TestObject> testObject = stackAllocator.template New<TestObject>(1, 1.5F, 'a', false, 2.5F);
        EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));

        objects.push_back(testObject);
    }

    // for (auto object : objects) {
    //     stackAllocator.Delete(object);
    // }
}

TEST_F(StackAllocatorTest, Multithreaded)
{
    constexpr StackAllocatorSettings settings = {.policy = StackAllocatorPolicy::Default | StackAllocatorPolicy::Multithreaded};

    StackAllocator<settings> stackAllocator{sizeof(TestObject) * 5 * 10000};

    std::thread thread1(&ThreadFunction<settings>, std::ref(stackAllocator));
    std::thread thread2(&ThreadFunction<settings>, std::ref(stackAllocator));
    std::thread thread3(&ThreadFunction<settings>, std::ref(stackAllocator));
    std::thread thread4(&ThreadFunction<settings>, std::ref(stackAllocator));

    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();

    EXPECT_EQ(stackAllocator.GetUsedSize(), sizeof(TestObject) * 4 * 10000);
}

TEST_F(StackAllocatorTest, Release)
{
    StackAllocator<> stackAllocator{10 * (sizeof(TestObject) + std::max(alignof(TestObject), std::size_t(8)))};
    for (int i = 0; i < 10; i++)
    {
        TestObject* object =
            CheckNewRaw<TestObject>(stackAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);
    }

    stackAllocator.Release();

    for (int i = 0; i < 10; i++)
    {
        TestObject* object =
            CheckNewRaw<TestObject>(stackAllocator, i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);
    }
}

TEST_F(StackAllocatorTest, GetUsedSizeNew)
{
    constexpr StackAllocatorSettings settings = {.policy = StackAllocatorPolicy::Default};
    StackAllocator<settings>         stackAllocator{1_MB};
    const int                        numObjects = 10;
    for (int i = 0; i < numObjects; i++)
    {
        TestObject* object =
            stackAllocator.NewRaw<TestObject>(i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);
    }

    EXPECT_EQ(stackAllocator.GetUsedSize(), numObjects * (sizeof(TestObject) + std::max(alignof(TestObject), size_t(8))));
}

ALLOCATOR_DEBUG_TEST(GetUsedSizeNewDelete, {
    const int                numObjects = 10;
    std::vector<TestObject*> objects;
    for (int i = 0; i < numObjects; i++)
    {
        TestObject* object =
            stackAllocator.NewRaw<TestObject>(i, static_cast<float>(i) + 1.5F, 'a' + i, i % 2, static_cast<float>(i) + 2.5F);
        objects.push_back(object);
    }

    for (int i = numObjects - 1; i >= 0; i--)
    {
        stackAllocator.Delete(objects[i]);
    }

    EXPECT_EQ(stackAllocator.GetUsedSize(), 0);
})

TEST_F(StackAllocatorTest, GetUsedSizeNewArray)
{
    constexpr StackAllocatorSettings settings = {.policy = StackAllocatorPolicy::Default};
    StackAllocator<settings>         stackAllocator{1_MB};

    const int   numObjects = 10;
    TestObject* arr        = stackAllocator.NewArrayRaw<TestObject>(numObjects, 1, 2.1F, 'a', false, 10.6F);

    EXPECT_EQ(stackAllocator.GetUsedSize(), std::max(alignof(TestObject), size_t(8) + numObjects * sizeof(TestObject)));
}

ALLOCATOR_DEBUG_TEST(GetUsedSizeNewDeleteArray, {
    const int   numObjects = 10;
    TestObject* arr        = stackAllocator.NewArrayRaw<TestObject>(numObjects, 1, 2.1F, 'a', false, 10.6F);

    stackAllocator.DeleteArray(arr);

    EXPECT_EQ(stackAllocator.GetUsedSize(), 0);
})

TEST_F(StackAllocatorTest, MemoryTracker)
{
    constexpr StackAllocatorSettings settings = {.policy = StackAllocatorPolicy::Debug};
    StackAllocator<settings>         stackAllocator{1_MB};

    int* num = static_cast<int*>(stackAllocator.Allocate<int>("Testing/StackAllocator"));

    const AllocatorVector allocators = MemoryTracker::GetAllocators();

    const Size size = sizeof(int) + sizeof(Internal::StackHeader) + sizeof(BoundGuardFront) + sizeof(BoundGuardBack);

    EXPECT_EQ(allocators.size(), 1);
    if (allocators.size() > 0)
    {
        EXPECT_EQ(allocators[0]->totalSize, 1_MB);
        EXPECT_EQ(allocators[0]->usedSize, size);
        EXPECT_EQ(allocators[0]->allocationCount, 1);
        EXPECT_EQ(allocators[0]->deallocationCount, 0);
        EXPECT_EQ(allocators[0]->allocations[0].category, std::string("Testing/StackAllocator"));
        EXPECT_EQ(allocators[0]->allocations[0].size, sizeof(int));
    }
}

ALLOCATOR_DEBUG_TEST(DefaultBaseAllocator, {
    int* num = static_cast<int*>(stackAllocator.Allocate<int>("Testing/StackAllocator"));
    EXPECT_EQ(Allocator::GetDefaultAllocator()->GetTotalSize(), 1_MB);
})

TEST_F(StackAllocatorTest, CustomBaseAllocator)
{
    constexpr MallocatorSettings mallocatorSettings = {.policy = MallocatorPolicy::Default};
    auto                         baseAllocator      = std::make_shared<Mallocator<mallocatorSettings>>("Mallocator");

    constexpr StackAllocatorSettings settings = {.policy = StackAllocatorPolicy::Default};
    StackAllocator<settings>         stackAllocator{1_MB, "TestAllocator", baseAllocator};

    int* num = static_cast<int*>(stackAllocator.Allocate<int>("Testing/StackAllocator"));
    EXPECT_EQ(baseAllocator->GetTotalSize(), 1_MB);
}

TEST_F(StackAllocatorTest, DoubleFreePreventionDisabled)
{
    constexpr StackAllocatorSettings settings = {.policy = StackAllocatorPolicy::Default & ~StackAllocatorPolicy::DoubleFreePrevention};

    StackAllocator<settings> stackAllocator{1_MB};
    auto                     ptr  = stackAllocator.New<TestObject>();
    auto                     ptr2 = stackAllocator.NewRaw<TestObject>();
    auto                     ptr3 = stackAllocator.Allocate(8);
    stackAllocator.Deallocate(ptr3);
    stackAllocator.Delete(ptr2);
    stackAllocator.Delete(ptr);
    EXPECT_NE(ptr, nullptr);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_NE(ptr3, nullptr);
}

TEST_F(StackAllocatorTest, DoubleFreePrevention)
{
    constexpr StackAllocatorSettings settings = {.policy = StackAllocatorPolicy::DoubleFreePrevention};

    StackAllocator<settings> stackAllocator{1_MB};
    auto                     ptr  = stackAllocator.New<TestObject>();
    auto                     ptr2 = stackAllocator.NewRaw<TestObject>();
    auto                     ptr3 = stackAllocator.Allocate(8);
    stackAllocator.Deallocate(ptr3);
    stackAllocator.Delete(ptr2);
    stackAllocator.Delete(ptr);
    EXPECT_EQ(ptr, nullptr);
    EXPECT_EQ(ptr2, nullptr);
    EXPECT_EQ(ptr3, nullptr);
}

#ifdef MEMARENA_ENABLE_ASSERTS

class StackAllocatorDeathTest : public ::testing::Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}

    StackAllocator<> stackAllocator = StackAllocator<>(10_MB);
};

TEST_F(StackAllocatorDeathTest, MaxSizeAllocation)
{
    // TODO Write proper exit messages
    ASSERT_DEATH({ StackAllocator stackAllocator{Size(std::numeric_limits<Offset>::max()) + 1}; }, ".*");
}

TEST_F(StackAllocatorDeathTest, NewOutOfMemory)
{
    StackAllocator stackAllocator2{10};

    // TODO Write proper exit messages
    ASSERT_DEATH({ TestObject* object = stackAllocator2.NewRaw<TestObject>(1, 2.1F, 'a', false, 10.6F); }, ".*");
}

TEST_F(StackAllocatorDeathTest, DeleteNullPointer)
{
    int* nullPointer = nullptr;

    // TODO Write proper exit messages
    ASSERT_DEATH({ stackAllocator.Delete(nullPointer); }, ".*");
}

TEST_F(StackAllocatorDeathTest, DeleteNotOwnedPointer)
{
    int* pointer = new int(10);

    // TODO Write proper exit messages
    ASSERT_DEATH({ stackAllocator.Delete(pointer); }, ".*");
}

TEST_F(StackAllocatorDeathTest, MemoryStompingDetection)
{
    constexpr StackAllocatorSettings settings = {.policy = StackAllocatorPolicy::BoundsCheck};
    StackAllocator<settings>         stackAllocator2{1_KB};

    TestObject* testObject = stackAllocator2.NewRaw<TestObject>(1, 1.5F, 'a', false, 2.5F);
    EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));

    TestObject* testObject2 = stackAllocator2.NewRaw<TestObject>(1, 1.5F, 'a', false, 2.5F);
    EXPECT_EQ(*testObject2, TestObject(1, 1.5F, 'a', false, 2.5F));

    stackAllocator2.Delete(testObject);

    TestObject2* testObject3 = stackAllocator2.NewRaw<TestObject2>(1, 1.5F, 'a', false, Pair{1, 2.5});
    EXPECT_EQ(*testObject3, TestObject2(1, 1.5F, 'a', false, Pair{1, 2.5}));

    // TODO Write proper exit messages
    stackAllocator2.Delete(testObject3);

    ASSERT_DEATH({ stackAllocator2.Delete(testObject2); }, ".*");
}

TEST_F(StackAllocatorDeathTest, DeleteWrongOrder)
{
    constexpr StackAllocatorSettings settings = {.policy = StackAllocatorPolicy::StackCheck};
    StackAllocator<settings>         stackAllocator2{1_KB};

    TestObject* testObject = stackAllocator2.NewRaw<TestObject>(1, 1.5F, 'a', false, 2.5F);
    EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5F));

    TestObject* testObject2 = stackAllocator2.NewRaw<TestObject>(1, 1.5F, 'a', false, 2.5F);
    EXPECT_EQ(*testObject2, TestObject(1, 1.5F, 'a', false, 2.5F));

    // TODO Write proper exit messages
    ASSERT_DEATH({ stackAllocator2.Delete(testObject); }, ".*");
}

#endif
