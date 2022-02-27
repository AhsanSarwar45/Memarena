#include <benchmark/benchmark.h>

#include <Memarena/StackAllocator.hpp>
#include <Memarena/StackAllocatorSafe.hpp>

#include "MemoryTestObjects.hpp"

using namespace Memarena;

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

static void StackAllocatorNewClear(benchmark::State& state)
{
    StackAllocator stackAllocator = StackAllocator(1000 * (8 + sizeof(TestObject)));

    for (auto _ : state)
    {
        for (size_t i = 0; i < 1000; i++)
        {
            TestObject* object = stackAllocator.New<TestObject>(1, 1.5f, 2.5f, false, 10.5f);
        }

        stackAllocator.Reset();
    }
}

BENCHMARK(StackAllocatorNewClear);

static void StackAllocatorNewDelete(benchmark::State& state)
{
    StackAllocator stackAllocator = StackAllocator(1000 * sizeof(TestObject));

    for (auto _ : state)
    {
        for (size_t i = 0; i < 1000; i++)
        {
            TestObject* object = stackAllocator.New<TestObject>(1, 1.5f, 2.5f, false, 10.5f);
            stackAllocator.Delete(object);
        }
    }
}

BENCHMARK(StackAllocatorNewDelete);

static void StackAllocatorSafeNewClear(benchmark::State& state)
{
    StackAllocatorSafe stackAllocatorSafe = StackAllocatorSafe(1000 * (8 + sizeof(TestObject)));

    for (auto _ : state)
    {
        for (size_t i = 0; i < 1000; i++)
        {
            StackPtr<TestObject> object = stackAllocatorSafe.New<TestObject>(1, 1.5f, 2.5f, false, 10.5f);
        }

        stackAllocatorSafe.Reset();
    }
}

BENCHMARK(StackAllocatorSafeNewClear);

static void StackAllocatorSafeNewDelete(benchmark::State& state)
{
    StackAllocatorSafe stackAllocatorSafe = StackAllocatorSafe(1000 * (8 + sizeof(TestObject)));

    for (auto _ : state)
    {
        for (size_t i = 0; i < 1000; i++)
        {
            StackPtr<TestObject> object = stackAllocatorSafe.New<TestObject>(1, 1.5f, 2.5f, false, 10.5f);
            stackAllocatorSafe.Delete(object);
        }
    }
}

BENCHMARK(StackAllocatorSafeNewDelete);
