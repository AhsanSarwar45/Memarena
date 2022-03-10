#include <benchmark/benchmark.h>

#include <Memarena/Memarena.hpp>

#include "MemoryTestObjects.hpp"

using namespace Memarena;

static void DefaultNewDelete(benchmark::State& state)
{
    for (auto _ : state)
    {
        TestObject* object = new TestObject(1, 1.5f, 'c', false, 10.5f);
        benchmark::ClobberMemory();
        delete object;
    }
}
BENCHMARK(DefaultNewDelete);

static void UniquePtr(benchmark::State& state)
{
    for (auto _ : state)
    {
        std::unique_ptr<TestObject> object = std::make_unique<TestObject>(1, 1.5f, 'c', false, 10.5f);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(UniquePtr);

static void StackAllocatorNewDeleteRaw(benchmark::State& state)
{
    StackAllocator stackAllocator = StackAllocator(8 + sizeof(TestObject));

    for (auto _ : state)
    {
        TestObject* object = stackAllocator.NewRaw<TestObject>(1, 1.5f, 'x', false, 10.5f);
        stackAllocator.Delete(object);
    }
}

BENCHMARK(StackAllocatorNewDeleteRaw);

static void StackAllocatorNewDelete(benchmark::State& state)
{
    StackAllocator stackAllocator = StackAllocator(8 + sizeof(TestObject));

    for (auto _ : state)
    {
        StackPtr<TestObject> object = stackAllocator.New<TestObject>(1, 1.5f, 'c', false, 10.5f);
        stackAllocator.Delete(object);
    }
}

BENCHMARK(StackAllocatorNewDelete);

static void LinearAllocatorNewResetRaw(benchmark::State& state)
{
    LinearAllocator linearAllocator = LinearAllocator(sizeof(TestObject));

    for (auto _ : state)
    {
        TestObject* object = linearAllocator.NewRaw<TestObject>(1, 1.5f, 'c', false, 10.5f);
        linearAllocator.Reset();
    }
}

BENCHMARK(LinearAllocatorNewResetRaw);