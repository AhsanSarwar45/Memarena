#include <benchmark/benchmark.h>

#include <Memarena/Memarena.hpp>

#include "MemoryTestObjects.hpp"
#include "Source/Allocators/Mallocator/Mallocator.hpp"
#include "Source/Allocators/StackAllocator/StackAllocator.hpp"
#include "Source/Policies/Policies.hpp"

using namespace Memarena;

static void DefaultNewDelete(benchmark::State& state)
{
    for (auto _ : state)
    {
        TestObject* object = new TestObject(1, 1.5F, 'c', false, 10.5F);
        benchmark::ClobberMemory();
        delete object;
    }
}
BENCHMARK(DefaultNewDelete);

static void UniquePtr(benchmark::State& state)
{
    for (auto _ : state)
    {
        std::unique_ptr<TestObject> object = std::make_unique<TestObject>(1, 1.5F, 'c', false, 10.5F);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(UniquePtr);

static void StackAllocatorNewDeleteRaw(benchmark::State& state)
{
    StackAllocator stackAllocator{sizeof(TestObject)};

    for (auto _ : state)
    {
        TestObject* object = stackAllocator.NewRaw<TestObject>(1, 1.5F, 'x', false, 10.5F);
        stackAllocator.Delete(object);
    }
}

BENCHMARK(StackAllocatorNewDeleteRaw);

static void StackAllocatorNewDelete(benchmark::State& state)
{
    StackAllocator stackAllocator{sizeof(TestObject)};

    for (auto _ : state)
    {
        StackPtr<TestObject> object = stackAllocator.New<TestObject>(1, 1.5F, 'c', false, 10.5F);
        stackAllocator.Delete(object);
    }
}

BENCHMARK(StackAllocatorNewDelete);

static void StackAllocatorTemplatedNewDelete(benchmark::State& state)
{
    StackAllocatorTemplated<TestObject> stackAllocatorTemplated{sizeof(TestObject)};

    for (auto _ : state)
    {
        StackPtr<TestObject> object = stackAllocatorTemplated.New(1, 1.5F, 'c', false, 10.5F);
        stackAllocatorTemplated.Delete(object);
    }
}

BENCHMARK(StackAllocatorTemplatedNewDelete);

static void StackAllocatorNewDeleteRawMultithreaded(benchmark::State& state)
{
    constexpr StackAllocatorSettings settings = {.policy = GetDefaultPolicy<StackAllocatorPolicy>() | StackAllocatorPolicy::Multithreaded};
    StackAllocator<settings>         stackAllocator{sizeof(TestObject)};

    for (auto _ : state)
    {
        TestObject* object = stackAllocator.NewRaw<TestObject>(1, 1.5F, 'c', false, 10.5F);
        stackAllocator.Delete(object);
    }
}

BENCHMARK(StackAllocatorNewDeleteRawMultithreaded);

static void StackAllocatorNewDeleteMultithreaded(benchmark::State& state)
{
    constexpr StackAllocatorSettings settings = {.policy = GetDefaultPolicy<StackAllocatorPolicy>() | StackAllocatorPolicy::Multithreaded};
    StackAllocator<settings>         stackAllocator{sizeof(TestObject)};

    for (auto _ : state)
    {
        StackPtr<TestObject> object = stackAllocator.New<TestObject>(1, 1.5F, 'c', false, 10.5F);
        stackAllocator.Delete(object);
    }
}

BENCHMARK(StackAllocatorNewDeleteMultithreaded);

static void LinearAllocatorNewReleaseRaw(benchmark::State& state)
{
    LinearAllocator linearAllocator{sizeof(TestObject)};

    for (auto _ : state)
    {
        TestObject* object = linearAllocator.NewRaw<TestObject>(1, 1.5F, 'c', false, 10.5F);
        linearAllocator.Release();
    }
}

BENCHMARK(LinearAllocatorNewReleaseRaw);

static void LinearAllocatorNewReleaseRawMultithreaded(benchmark::State& state)
{
    constexpr LinearAllocatorSettings settings = {.policy =
                                                      GetDefaultPolicy<LinearAllocatorPolicy>() | LinearAllocatorPolicy::Multithreaded};

    LinearAllocator<settings> linearAllocator{sizeof(TestObject)};

    for (auto _ : state)
    {
        TestObject* object = linearAllocator.NewRaw<TestObject>(1, 1.5F, 'c', false, 10.5F);
        linearAllocator.Release();
    }
}

BENCHMARK(LinearAllocatorNewReleaseRawMultithreaded);

static void MallocatorNewDelete(benchmark::State& state)
{
    Mallocator mallocator{};

    for (auto _ : state)
    {
        MallocPtr<TestObject> object = mallocator.New<TestObject>(1, 1.5F, 'c', false, 10.5F);
        benchmark::ClobberMemory();
        mallocator.Delete(object);
    }
}

BENCHMARK(MallocatorNewDelete);
