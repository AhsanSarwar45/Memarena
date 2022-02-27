#include <benchmark/benchmark.h>

#include <Memarena/StackAllocator.hpp>
#include <Memarena/StackAllocatorSafe.hpp>

#include "MemoryTestObjects.hpp"

using namespace Memarena;

static void DefaultAccess(benchmark::State& state)
{
    std::vector<TestObject*> objects = std::vector<TestObject*>(20);

    for (size_t i = 0; i < 20; i++)
    {
        objects[i] = new TestObject(1, 1.5f, 2.5f, false, 10.5f);
    }

    for (auto _ : state)
    {

        int num = 0;
        for (size_t i = 0; i < 20; i++)
        {
            num += objects[i]->a + objects[i]->b + objects[i]->e;
        }
        benchmark::DoNotOptimize(num);
        benchmark::ClobberMemory();
    }

    for (size_t i = 0; i < 20; i++)
    {
        delete objects[i];
    }
}
BENCHMARK(DefaultAccess);

static void StackAllocatorAccess(benchmark::State& state)
{
    StackAllocator           stackAllocator = StackAllocator(2_KB);
    std::vector<TestObject*> objects        = std::vector<TestObject*>(20);

    for (size_t i = 0; i < 20; i++)
    {
        objects[i] = stackAllocator.New<TestObject>(1, 1.5f, 2.5f, false, 10.5f);
    }

    for (auto _ : state)
    {

        int num = 0;
        for (size_t i = 0; i < 20; i++)
        {
            num += objects[i]->a + objects[i]->b + objects[i]->e;
        }
        benchmark::DoNotOptimize(num);
        benchmark::ClobberMemory();
    }

    for (size_t i = 0; i < 20; i++)
    {
        stackAllocator.Delete(objects[i]);
    }
}

BENCHMARK(StackAllocatorAccess);

static void StackAllocatorSafeAccess(benchmark::State& state)
{
    StackAllocatorSafe                stackAllocatorSafe = StackAllocatorSafe(2_KB);
    std::vector<StackPtr<TestObject>> objects            = std::vector<StackPtr<TestObject>>(20);

    for (size_t i = 0; i < 20; i++)
    {
        objects[i] = stackAllocatorSafe.New<TestObject>(1, 1.5f, 2.5f, false, 10.5f);
    }

    for (auto _ : state)
    {
        int num = 0;
        for (size_t i = 0; i < 20; i++)
        {
            num += objects[i]->a + objects[i]->b + objects[i]->e;
        }
        benchmark::DoNotOptimize(num);
        benchmark::ClobberMemory();
    }

    for (size_t i = 0; i < 20; i++)
    {
        stackAllocatorSafe.Delete(objects[i]);
    }
}

BENCHMARK(StackAllocatorSafeAccess);
