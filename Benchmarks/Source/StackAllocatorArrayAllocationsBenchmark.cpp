#include <benchmark/benchmark.h>

#include <Memarena/Memarena.hpp>

#include <algorithm>

#include "MemoryTestObjects.hpp"

using namespace Memarena;

const int NUM_OBJECTS = 10;

static void DefaultNewDeleteArray(benchmark::State& state)
{
    for (auto _ : state)
    {
        TestObject* arr = new TestObject[NUM_OBJECTS];
        for (size_t i = 0; i < NUM_OBJECTS; i++)
        {
            arr[i] = TestObject(1, 1.5f, 2.5f, false, 10.5f);
        }

        benchmark::ClobberMemory();
        delete arr;
    }
}
BENCHMARK(DefaultNewDeleteArray);

static void StackAllocatorNewDeleteArray(benchmark::State& state)
{
    StackAllocator stackAllocator = StackAllocator(8 + NUM_OBJECTS * sizeof(TestObject));

    for (auto _ : state)
    {
        TestObject* arr = stackAllocator.NewArray<TestObject>(NUM_OBJECTS, 1, 1.5f, 2.5f, false, 10.5f);
        stackAllocator.DeleteArray(arr);
    }
}

BENCHMARK(StackAllocatorNewDeleteArray);

static void StackAllocatorSafeNewDeleteArray(benchmark::State& state)
{
    StackAllocatorSafe stackAllocatorSafe = StackAllocatorSafe(NUM_OBJECTS * sizeof(TestObject));

    for (auto _ : state)
    {
        StackPtr<TestObject> arr = stackAllocatorSafe.NewArray<TestObject>(NUM_OBJECTS, 1, 1.5f, 2.5f, false, 10.5f);
        stackAllocatorSafe.DeleteArray(arr);
    }
}

BENCHMARK(StackAllocatorSafeNewDeleteArray);
