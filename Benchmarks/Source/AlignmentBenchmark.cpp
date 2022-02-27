#include <benchmark/benchmark.h>

#include <Memarena/Utility/Alignment.hpp>

#include "MemoryTestObjects.hpp"

static void CalculateAlignedAddress(benchmark::State& state)
{

    for (auto _ : state)
    {
        Memarena::UIntPtr alignedAddress = Memarena::CalculateAlignedAddress(1001, 4);

        benchmark::DoNotOptimize(alignedAddress);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(CalculateAlignedAddress);

static void CalculateShortestAlignedPadding(benchmark::State& state)
{

    for (auto _ : state)
    {
        Memarena::UIntPtr alignedPadding = Memarena::CalculateShortestAlignedPadding(1001, 4);

        benchmark::DoNotOptimize(alignedPadding);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(CalculateShortestAlignedPadding);

static void CalculateAlignedPaddingWithHeader(benchmark::State& state)
{

    for (auto _ : state)
    {
        Memarena::UIntPtr alignedPadding = Memarena::CalculateAlignedPaddingWithHeader(1001, 4, 8);

        benchmark::DoNotOptimize(alignedPadding);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(CalculateAlignedPaddingWithHeader);
