#include <benchmark/benchmark.h>

#include <Memarena/Memarena.hpp>

#include <algorithm>

#include "MemoryTestObjects.hpp"
#include "Source/Policies/Policies.hpp"

using namespace Memarena;

const int NUM_OBJECTS = 10;

static void DefaultNewDeleteArray(benchmark::State& state)
{
    for (auto _ : state)
    {
        TestObject* arr = new TestObject[NUM_OBJECTS];
        for (size_t i = 0; i < NUM_OBJECTS; i++)
        {
            arr[i] = TestObject(1, 1.5f, 'c', false, 10.5f);
        }

        benchmark::ClobberMemory();
        delete[] arr;
    }
}
BENCHMARK(DefaultNewDeleteArray);

static void StackAllocatorRawNewDeleteArray(benchmark::State& state)
{
    StackAllocator<StackAllocatorPolicy::Release> stackAllocator{8 + NUM_OBJECTS * sizeof(TestObject)};

    for (auto _ : state)
    {
        TestObject* arr = stackAllocator.NewArrayRaw<TestObject>(NUM_OBJECTS, 1, 1.5f, 'c', false, 10.5f);
        stackAllocator.DeleteArray(arr);
    }
}

BENCHMARK(StackAllocatorRawNewDeleteArray);

static void StackAllocatorNewDeleteArray(benchmark::State& state)
{
    StackAllocator<StackAllocatorPolicy::Release> stackAllocator{8 + NUM_OBJECTS * sizeof(TestObject)};

    for (auto _ : state)
    {
        auto arr = stackAllocator.NewArray<TestObject>(NUM_OBJECTS, 1, 1.5f, 'c', false, 10.5f);
        stackAllocator.DeleteArray(arr);
    }
}

BENCHMARK(StackAllocatorNewDeleteArray);

static void LinearAllocatorRawNewReleaseArray(benchmark::State& state)
{
    LinearAllocator<LinearAllocatorPolicy::Release> linearAllocator{8 + NUM_OBJECTS * sizeof(TestObject)};

    for (auto _ : state)
    {
        auto arr = linearAllocator.NewArrayRaw<TestObject>(NUM_OBJECTS, 1, 1.5f, 'c', false, 10.5f);
        linearAllocator.Release();
    }
}

BENCHMARK(LinearAllocatorRawNewReleaseArray);

static void MallocatorNewDeleteArray(benchmark::State& state)
{
    Mallocator<MallocatorPolicy::Release> mallocator{};

    for (auto _ : state)
    {
        auto arr = mallocator.NewArray<TestObject>(NUM_OBJECTS, 1, 1.5f, 'c', false, 10.5f);
        benchmark::ClobberMemory();
        mallocator.DeleteArray(arr);
    }
}

BENCHMARK(MallocatorNewDeleteArray);
