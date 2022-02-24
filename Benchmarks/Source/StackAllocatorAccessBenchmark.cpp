#include <benchmark/benchmark.h>

#include <MemoryManager/StackAllocator.hpp>
#include <MemoryManager/StackAllocatorSafe.hpp>

#include "MemoryTestObjects.hpp"

using namespace Memory;

static void DefaultAccess(benchmark::State& state)
{
    std::vector<TestObject*> objects = std::vector<TestObject*>(20);

    for (size_t i = 0; i < 20; i++)
    {
        objects[i] = new TestObject(1, 1.5f, 2.5f, false, 10.5f);
    }

    for (auto _ : state)
    {
        for (size_t i = 0; i < 1000; i++)
        {
            int num = 0;
            for (size_t i = 0; i < 20; i++)
            {
                num += objects[i]->a + objects[i]->b + objects[i]->e;
            }
            benchmark::DoNotOptimize(num);
            benchmark::ClobberMemory();
        }
    }

    for (size_t i = 0; i < 20; i++)
    {
        delete objects[i];
    }
}
BENCHMARK(DefaultAccess);

static void StackAllocatorAccessAlign(benchmark::State& state)
{
    StackAllocator           stackAllocator = StackAllocator(2_KB, nullptr, 1);
    std::vector<TestObject*> objects        = std::vector<TestObject*>(20);

    for (size_t i = 0; i < 20; i++)
    {
        objects[i] = stackAllocator.New<TestObject>(1, 1.5f, 2.5f, false, 10.5f);
    }

    for (auto _ : state)
    {
        for (size_t i = 0; i < 1000; i++)
        {
            int num = 0;
            for (size_t i = 0; i < 20; i++)
            {
                num += objects[i]->a + objects[i]->b + objects[i]->e;
            }
            benchmark::DoNotOptimize(num);
            benchmark::ClobberMemory();
        }
    }

    for (size_t i = 0; i < 20; i++)
    {
        stackAllocator.Delete(objects[i]);
    }
}

BENCHMARK(StackAllocatorAccessAlign)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(11);
