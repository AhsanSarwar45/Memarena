# ===================================================
# BUILD SYSTEM

# This is the CMakeLists.txt that generates the
# executable for the Benchmarks
# ===================================================

project(Benchmarks)

set(BENCHMARK_CPP_LINKER_FLAGS "-lpthread")

add_executable(${PROJECT_NAME} 
"Source/Main.cpp"
"Source/StackAllocatorBenchmark.cpp"
)

target_include_directories(${PROJECT_NAME} PUBLIC "Source")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/Include")

target_link_libraries(${PROJECT_NAME} PRIVATE "${BENCHMARK_CPP_LINKER_FLAGS}")
target_link_libraries(${PROJECT_NAME} PRIVATE MemoryManager)
target_link_libraries(${PROJECT_NAME} PRIVATE benchmark::benchmark_main)

