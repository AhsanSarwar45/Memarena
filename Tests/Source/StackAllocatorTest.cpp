#include <functional>
#include <gtest/gtest.h>

#include <thread>

#include <Memarena/Memarena.hpp>

#include "Macro.hpp"
#include "MemoryTestObjects.hpp"
#include "Source/AllocatorData.hpp"

using namespace Memarena;
using namespace Memarena::SizeLiterals;

class StackAllocatorTest : public ::testing::Test
{
  protected:
    void SetUp() override { MemoryTracker::Reset(); }
    void TearDown() override {}

    StackAllocator<> stackAllocator = StackAllocator<>(10_MB);
};

template <typename Object, typename... Args>
Object* CheckNewRaw(StackAllocator<>& allocator, Args&&... argList)
{
    Object* object = allocator.NewRaw<Object>(std::forward<Args>(argList)...);

    EXPECT_EQ(*object, Object(std::forward<Args>(argList)...));

    return object;
}
template <typename Object, typename... Args>
Object* CheckNewArrayRaw(StackAllocator<>& allocator, size_t objectCount, Args&&... argList)
{
    Object* arr = allocator.NewArrayRaw<Object>(objectCount, std::forward<Args>(argList)...);

    for (size_t i = 0; i < objectCount; i++)
    {
        EXPECT_EQ(arr[i], Object(std::forward<Args>(argList)...));
    }
    return arr;
}

template <typename Object, typename... Args>
Memarena::StackPtr<Object> CheckNew(StackAllocator<>& allocator, Args&&... argList)
{
    Memarena::StackPtr<Object> object = allocator.New<Object>(std::forward<Args>(argList)...);

    EXPECT_EQ(*(object.GetPtr()), Object(std::forward<Args>(argList)...));

    return object;
}

template <typename Object, typename... Args>
Memarena::StackArrayPtr<Object> CheckNewArray(StackAllocator<>& allocator, size_t objectCount, Args&&... argList)
{
    Memarena::StackArrayPtr<Object> arr = allocator.NewArray<Object>(objectCount, std::forward<Args>(argList)...);

    for (size_t i = 0; i < objectCount; i++)
    {
        EXPECT_EQ(arr[i], Object(std::forward<Args>(argList)...));
    }
    return arr;
}

TEST_F(StackAllocatorTest, Initialize) { EXPECT_EQ(stackAllocator.GetUsedSize(), 0); }

TEST_F(StackAllocatorTest, RawNewSingleObject) { CheckNewRaw<TestObject>(stackAllocator, 1, 2.1f, 'a', false, 10.6f); }

TEST_F(StackAllocatorTest, RawNewMultipleObjects)
{
    for (size_t i = 0; i < 10; i++)
    {
        CheckNewRaw<TestObject>(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }
    for (size_t i = 0; i < 10; i++)
    {
        CheckNewRaw<TestObject2>(stackAllocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2.5});
    }
}

TEST_F(StackAllocatorTest, RawNewDeleteSingleObject)
{
    TestObject* object = CheckNewRaw<TestObject>(stackAllocator, 1, 2.1f, 'a', false, 10.6f);

    stackAllocator.Delete(object);
}

TEST_F(StackAllocatorTest, RawNewDeleteMultipleObjects)
{
    std::vector<TestObject*>  objects1;
    std::vector<TestObject2*> objects2;

    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckNewRaw<TestObject>(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);

        objects1.push_back(object);
    }
    for (size_t i = 0; i < 10; i++)
    {
        TestObject2* object = CheckNewRaw<TestObject2>(stackAllocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2});

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

TEST_F(StackAllocatorTest, RawNewDeleteNewSingleObject)
{
    TestObject* object = CheckNewRaw<TestObject>(stackAllocator, 1, 2.1f, 'a', false, 10.6f);

    stackAllocator.Delete(object);

    TestObject* object2 = CheckNewRaw<TestObject>(stackAllocator, 1, 2.1f, 'a', false, 10.6f);
}

TEST_F(StackAllocatorTest, RawNewDeleteNewMultipleObjects)
{
    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckNewRaw<TestObject>(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);

        stackAllocator.Delete(object);
    }
    for (size_t i = 0; i < 10; i++)
    {
        TestObject2* object = CheckNewRaw<TestObject2>(stackAllocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2.5});

        stackAllocator.Delete(object);
    }
}

TEST_F(StackAllocatorTest, RawNewArrayRaw)
{
    TestObject* arr = CheckNewArrayRaw<TestObject>(stackAllocator, 10, 1, 2.1f, 'a', false, 10.6f);
}

TEST_F(StackAllocatorTest, RawNewDeleteArray)
{
    TestObject* arr = CheckNewArrayRaw<TestObject>(stackAllocator, 10, 1, 2.1f, 'a', false, 10.6f);
    stackAllocator.DeleteArray(arr);
}

TEST_F(StackAllocatorTest, RawNewMixed)
{
    TestObject* arr1    = CheckNewArrayRaw<TestObject>(stackAllocator, 10, 1, 2.1f, 'a', false, 10.6f);
    TestObject* object1 = CheckNewRaw<TestObject>(stackAllocator, 1, 2.1f, 'a', false, 10.6f);
    TestObject* object2 = CheckNewRaw<TestObject>(stackAllocator, 1, 2.1f, 'a', false, 10.6f);
    TestObject* arr2    = CheckNewArrayRaw<TestObject>(stackAllocator, 10, 1, 2.1f, 'a', false, 10.6f);
}

TEST_F(StackAllocatorTest, RawNewDeleteMixed)
{
    TestObject* arr1    = CheckNewArrayRaw<TestObject>(stackAllocator, 10, 1, 2.1f, 'a', false, 10.6f);
    TestObject* object1 = CheckNewRaw<TestObject>(stackAllocator, 1, 2.1f, 'a', false, 10.6f);
    TestObject* object2 = CheckNewRaw<TestObject>(stackAllocator, 1, 2.1f, 'a', false, 10.6f);
    TestObject* arr2    = CheckNewArrayRaw<TestObject>(stackAllocator, 10, 1, 2.1f, 'a', false, 10.6f);

    stackAllocator.DeleteArray(arr2);
    stackAllocator.Delete(object2);
    stackAllocator.Delete(object1);
    stackAllocator.DeleteArray(arr1);
}

TEST_F(StackAllocatorTest, NewSingleObject) { StackPtr object = CheckNew<TestObject>(stackAllocator, 1, 2.1f, 'a', false, 10.6f); }

TEST_F(StackAllocatorTest, NewMultipleObjects)
{
    for (size_t i = 0; i < 10; i++)
    {
        StackPtr<TestObject> object = CheckNew<TestObject>(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }
    for (size_t i = 0; i < 10; i++)
    {
        StackPtr<TestObject2> object = CheckNew<TestObject2>(stackAllocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2.5});
    }
}

TEST_F(StackAllocatorTest, NewDeleteSingleObject)
{
    StackPtr object = CheckNew<TestObject>(stackAllocator, 1, 2.1f, 'a', false, 10.6f);

    stackAllocator.Delete(object);
}

TEST_F(StackAllocatorTest, NewDeleteMultipleObjects)
{
    std::vector<StackPtr<TestObject>>  objects1;
    std::vector<StackPtr<TestObject2>> objects2;

    for (size_t i = 0; i < 10; i++)
    {
        StackPtr<TestObject> object = CheckNew<TestObject>(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);

        objects1.push_back(object);
    }
    for (size_t i = 0; i < 10; i++)
    {
        StackPtr<TestObject2> object = CheckNew<TestObject2>(stackAllocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2.5});

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

TEST_F(StackAllocatorTest, NewDeleteNewSingleObject)
{
    StackPtr<TestObject> object = CheckNew<TestObject>(stackAllocator, 1, 2.1f, 'a', false, 10.6f);

    stackAllocator.Delete(object);

    StackPtr<TestObject> object2 = CheckNew<TestObject>(stackAllocator, 1, 2.1f, 'a', false, 10.6f);
}

TEST_F(StackAllocatorTest, NewDeleteNewMultipleObjects)
{
    for (size_t i = 0; i < 10; i++)
    {
        StackPtr<TestObject> object = CheckNew<TestObject>(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);

        stackAllocator.Delete(object);
    }
    for (size_t i = 0; i < 10; i++)
    {
        StackPtr<TestObject2> object = CheckNew<TestObject2>(stackAllocator, i, i + 1.5, i + 2.5, i % 2, Pair{1, 2.5});

        stackAllocator.Delete(object);
    }
}

TEST_F(StackAllocatorTest, NewArray)
{
    StackArrayPtr<TestObject> arr = CheckNewArray<TestObject>(stackAllocator, 10, 1, 2.1f, 'a', false, 10.6f);
}

TEST_F(StackAllocatorTest, NewDeleteArray)
{
    StackArrayPtr<TestObject> arr = CheckNewArray<TestObject>(stackAllocator, 10, 1, 2.1f, 'a', false, 10.6f);
    stackAllocator.DeleteArray(arr);
}

TEST_F(StackAllocatorTest, NewMixed)
{
    StackArrayPtr<TestObject> arr1    = CheckNewArray<TestObject>(stackAllocator, 10, 1, 2.1f, 'a', false, 10.6f);
    StackPtr<TestObject>      object1 = CheckNew<TestObject>(stackAllocator, 1, 2.1f, 'a', false, 10.6f);
    StackPtr<TestObject>      object2 = CheckNew<TestObject>(stackAllocator, 1, 2.1f, 'a', false, 10.6f);
    StackArrayPtr<TestObject> arr2    = CheckNewArray<TestObject>(stackAllocator, 10, 1, 2.1f, 'a', false, 10.6f);
}

TEST_F(StackAllocatorTest, NewDeleteMixed)
{
    StackArrayPtr<TestObject> arr1    = CheckNewArray<TestObject>(stackAllocator, 10, 1, 2.1f, 'a', false, 10.6f);
    StackPtr<TestObject>      object1 = CheckNew<TestObject>(stackAllocator, 1, 2.1f, 'a', false, 10.6f);
    StackPtr<TestObject>      object2 = CheckNew<TestObject>(stackAllocator, 1, 2.1f, 'a', false, 10.6f);
    StackArrayPtr<TestObject> arr2    = CheckNewArray<TestObject>(stackAllocator, 10, 1, 2.1f, 'a', false, 10.6f);

    stackAllocator.DeleteArray(arr2);
    stackAllocator.Delete(object2);
    stackAllocator.Delete(object1);
    stackAllocator.DeleteArray(arr1);
}

TEST_F(StackAllocatorTest, Templated)
{
    StackAllocatorTemplated<TestObject> stackAllocatorTemplated{10_KB};

    TestObject* testObject = stackAllocatorTemplated.NewRaw(1, 1.5f, 'a', false, 2.5f);
    EXPECT_EQ(*testObject, TestObject(1, 1.5f, 'a', false, 2.5f));
    StackPtr<TestObject> testObject2 = stackAllocatorTemplated.New(1, 1.5f, 'a', false, 2.5f);
    EXPECT_EQ(*testObject, TestObject(1, 1.5f, 'a', false, 2.5f));

    TestObject* testObject3 = stackAllocatorTemplated.NewArrayRaw(10, 1, 1.5f, 'a', false, 2.5f);
    EXPECT_EQ(*testObject, TestObject(1, 1.5f, 'a', false, 2.5f));
    StackArrayPtr<TestObject> testObject4 = stackAllocatorTemplated.NewArray(10, 1, 1.5f, 'a', false, 2.5f);
    EXPECT_EQ(*testObject, TestObject(1, 1.5f, 'a', false, 2.5f));

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

template <StackAllocatorPolicy policy>
void ThreadFunction(StackAllocator<policy>& stackAllocator)
{
    std::vector<StackPtr<TestObject>> objects;

    for (size_t i = 0; i < 10000; i++)
    {
        StackPtr<TestObject> testObject = stackAllocator.template New<TestObject>(1, 1.5f, 'a', false, 2.5f);
        EXPECT_EQ(*testObject, TestObject(1, 1.5F, 'a', false, 2.5f));

        objects.push_back(testObject);
    }
}

TEST_F(StackAllocatorTest, NewMultithreaded)
{
    constexpr StackAllocatorPolicy policy = StackAllocatorPolicy::Default | StackAllocatorPolicy::Multithreaded;

    StackAllocator<policy> stackAllocator2{sizeof(TestObject) * 5 * 10000};

    std::thread thread1(&ThreadFunction<policy>, std::ref(stackAllocator2));
    std::thread thread2(&ThreadFunction<policy>, std::ref(stackAllocator2));
    std::thread thread3(&ThreadFunction<policy>, std::ref(stackAllocator2));
    std::thread thread4(&ThreadFunction<policy>, std::ref(stackAllocator2));

    thread1.join();
    thread2.join();
    thread3.join();
    thread4.join();

    EXPECT_EQ(stackAllocator2.GetUsedSize(), sizeof(TestObject) * 4 * 10000);
}

TEST_F(StackAllocatorTest, Release)
{
    StackAllocator<> stackAllocator2 = StackAllocator<>(10 * (sizeof(TestObject) + std::max(alignof(TestObject), std::size_t(8))));
    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckNewRaw<TestObject>(stackAllocator2, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }

    stackAllocator2.Release();

    for (size_t i = 0; i < 10; i++)
    {
        TestObject* object = CheckNewRaw<TestObject>(stackAllocator2, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }
}

TEST_F(StackAllocatorTest, GetUsedSizeNew)
{
    const int numObjects = 10;
    for (size_t i = 0; i < numObjects; i++)
    {
        TestObject* object = CheckNewRaw<TestObject>(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
    }

    EXPECT_EQ(stackAllocator.GetUsedSize(), numObjects * (sizeof(TestObject) + std::max(alignof(TestObject), size_t(8))));
}

TEST_F(StackAllocatorTest, GetUsedSizeNewDelete)
{
    const int                numObjects = 10;
    std::vector<TestObject*> objects;
    for (size_t i = 0; i < numObjects; i++)
    {
        TestObject* object = CheckNewRaw<TestObject>(stackAllocator, i, i + 1.5f, 'a' + i, i % 2, i + 2.5f);
        objects.push_back(object);
    }

    for (int i = numObjects - 1; i >= 0; i--)
    {
        stackAllocator.Delete(objects[i]);
    }

    EXPECT_EQ(stackAllocator.GetUsedSize(), 0);
}

TEST_F(StackAllocatorTest, GetUsedSizeNewArray)
{
    const int numObjects = 10;

    TestObject* arr = CheckNewArrayRaw<TestObject>(stackAllocator, numObjects, 1, 2.1f, 'a', false, 10.6f);

    EXPECT_EQ(stackAllocator.GetUsedSize(), std::max(alignof(TestObject), size_t(8) + numObjects * sizeof(TestObject)));
}

TEST_F(StackAllocatorTest, GetUsedSizeNewDeleteArray)
{
    const int   numObjects = 10;
    TestObject* arr        = CheckNewArrayRaw<TestObject>(stackAllocator, numObjects, 1, 2.1f, 'a', false, 10.6f);

    stackAllocator.DeleteArray(arr);

    EXPECT_EQ(stackAllocator.GetUsedSize(), 0);
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
    ASSERT_DEATH({ StackAllocator stackAllocator2{Size(std::numeric_limits<Offset>::max()) + 1}; }, ".*");
}

TEST_F(StackAllocatorDeathTest, NewOutOfMemory)
{
    StackAllocator stackAllocator2{10};

    // TODO Write proper exit messages
    ASSERT_DEATH({ TestObject* object = stackAllocator2.NewRaw<TestObject>(1, 2.1f, 'a', false, 10.6f); }, ".*");
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
    StackAllocator<StackAllocatorPolicy::BoundsCheck> stackAllocator2{1_KB};

    TestObject* testObject = stackAllocator2.NewRaw<TestObject>(1, 1.5f, 'a', false, 2.5f);
    EXPECT_EQ(*testObject, TestObject(1, 1.5f, 'a', false, 2.5f));

    TestObject* testObject2 = stackAllocator2.NewRaw<TestObject>(1, 1.5f, 'a', false, 2.5f);
    EXPECT_EQ(*testObject2, TestObject(1, 1.5f, 'a', false, 2.5f));

    stackAllocator2.Delete(testObject);

    TestObject2* testObject3 = stackAllocator2.NewRaw<TestObject2>(1, 1.5f, 'a', false, Pair{1, 2.5});
    EXPECT_EQ(*testObject3, TestObject2(1, 1.5f, 'a', false, Pair{1, 2.5}));

    // TODO Write proper exit messages
    stackAllocator2.Delete(testObject3);

    ASSERT_DEATH({ stackAllocator2.Delete(testObject2); }, ".*");
}

TEST_F(StackAllocatorDeathTest, DeleteWrongOrder)
{
    StackAllocator<StackAllocatorPolicy::StackCheck> stackAllocator2{1_KB};

    TestObject* testObject = stackAllocator2.NewRaw<TestObject>(1, 1.5f, 'a', false, 2.5f);
    EXPECT_EQ(*testObject, TestObject(1, 1.5f, 'a', false, 2.5f));

    TestObject* testObject2 = stackAllocator2.NewRaw<TestObject>(1, 1.5f, 'a', false, 2.5f);
    EXPECT_EQ(*testObject2, TestObject(1, 1.5f, 'a', false, 2.5f));

    // TODO Write proper exit messages
    ASSERT_DEATH({ stackAllocator2.Delete(testObject); }, ".*");
}

#endif
