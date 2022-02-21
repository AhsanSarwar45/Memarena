#include <benchmark/benchmark.h>

#include <MemoryManager/StackAllocator.hpp>

#include "MemoryTestObjects.hpp"

using namespace Memory;

static void DefaultNewDelete(benchmark::State& state)
{
    for (auto _ : state)
    {
        for (size_t i = 0; i < 1000; i++)
        {
            TestObject* object = new TestObject(1, 1.5f, 2.5f, false, 10.5f);
            benchmark::ClobberMemory();
            delete object;
        }
    }
}
BENCHMARK(DefaultNewDelete);

static void StackAllocatorNewDelete(benchmark::State& state)
{
    StackAllocator stackAllocator = StackAllocator(100 * (8 + sizeof(TestObject)));

    for (auto _ : state)
    {
        for (size_t i = 0; i < 1000; i++)
        {
            TestObject* object = stackAllocator.New<TestObject>(1, 1.5f, 2.5f, false, 10.5f);
        }

        stackAllocator.Reset();
    }
}
// Register the function as a benchmark
BENCHMARK(StackAllocatorNewDelete);
