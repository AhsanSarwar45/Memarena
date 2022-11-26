project('Memarena', 'cpp', version : '0.1.0', default_options : ['cpp_std=c++20'])

if get_option('buildtype').startswith('debug')
  add_project_arguments('-DMEMARENA_DEBUG', language : 'cpp')
endif

# ======== MEMARENA LIBRARY ========

sources = [
'Source/Allocator.cpp',
'Source/AllocatorUtils.cpp',
'Source/MemoryTracker.cpp',
'Source/Utility/Alignment/Alignment.cpp',
'Source/Utility/VirtualMemory.cpp'
]

dependencies = []

include_dir = [include_directories('Source')]

memarena_lib = library('Memarena', sources: sources, include_directories: include_dir, cpp_pch : 'Source/PCH.hpp')

memarena_dep = declare_dependency(
    include_directories : [include_directories('Include')],
    link_with : memarena_lib
    )

# ======== TESTS ========

test_sources = [
'Tests/Source/Main.cpp',
'Tests/Source/StackAllocatorTest.cpp',
'Tests/Source/LinearAllocatorTest.cpp',
'Tests/Source/PoolAllocatorTest.cpp',
'Tests/Source/MallocatorTest.cpp',
'Tests/Source/AlignmentTest.cpp',
'Tests/Source/MemoryTrackerTest.cpp'
]

gtest_dep = dependency('gtest')
test_dependencies = [gtest_dep, memarena_dep]

test_exe = executable('MemarenaTests', sources: test_sources , dependencies : test_dependencies)
test('gtest tests', test_exe)

# ======== BENCHMARKS ========

benchmark_sources = [
'Benchmarks/Source/Main.cpp',
'Benchmarks/Source/AllocationsBenchmark.cpp',
'Benchmarks/Source/StackAllocatorArrayAllocationsBenchmark.cpp',
'Benchmarks/Source/StackAllocatorAccessBenchmark.cpp',
'Benchmarks/Source/AlignmentBenchmark.cpp',
]

benchmark_dep = dependency('benchmark')
benchmark_dependencies = [benchmark_dep, memarena_dep]

benchmark_exe = executable('MemarenaBenchmarks', sources: benchmark_sources , dependencies : benchmark_dependencies)