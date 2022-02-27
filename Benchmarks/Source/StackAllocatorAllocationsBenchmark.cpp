#include <benchmark/benchmark.h>

#include <Memarena/StackAllocator.hpp>
#include <Memarena/StackAllocatorSafe.hpp>

#include "MemoryTestObjects.hpp"

using namespace Memarena;

static void DefaultNewDelete(benchmark::State& state)
{
    for (auto _ : state)
    {
        TestObject* object = new TestObject(1, 1.5f, 2.5f, false, 10.5f);
        benchmark::ClobberMemory();
        delete object;
    }
}
BENCHMARK(DefaultNewDelete);

static void UniquePtr(benchmark::State& state)
{
    for (auto _ : state)
    {
        std::unique_ptr<TestObject> object = std::make_unique<TestObject>(1, 1.5f, 2.5f, false, 10.5f);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(UniquePtr);

static void StackAllocatorNewDelete(benchmark::State& state)
{
    StackAllocator stackAllocator = StackAllocator(8 + sizeof(TestObject));

    for (auto _ : state)
    {
        TestObject* object = stackAllocator.New<TestObject>(1, 1.5f, 2.5f, false, 10.5f);
        stackAllocator.Delete(object);
    }
}

BENCHMARK(StackAllocatorNewDelete);

static void StackAllocatorSafeNewDelete(benchmark::State& state)
{
    StackAllocatorSafe stackAllocatorSafe = StackAllocatorSafe(sizeof(TestObject));

    for (auto _ : state)
    {
        StackPtr<TestObject> object = stackAllocatorSafe.New<TestObject>(1, 1.5f, 2.5f, false, 10.5f);
        stackAllocatorSafe.Delete(object);
    }
}

BENCHMARK(StackAllocatorSafeNewDelete);
