# ===================================================
# BUILD SYSTEM

# This is the CMakeLists.txt that generates the
# executable for the Benchmarks
# ===================================================

project(Benchmarks)

set(BENCHMARK_CPP_LINKER_FLAGS "-lpthread")

conan_cmake_configure(REQUIRES gtest/1.10.0 benchmark/1.6.0
                      OPTIONS gtest:build_gmock=False
                      GENERATORS cmake_find_package cmake_paths)

conan_cmake_autodetect(settings)

conan_cmake_install(PATH_OR_REFERENCE .
                    BUILD missing
                    REMOTE conancenter
                    SETTINGS ${settings})

add_executable(${PROJECT_NAME} 
"Source/Main.cpp"
"Source/StackAllocatorBenchmark.cpp"
)

include("${CMAKE_BINARY_DIR}/${PROJECT_NAME}/conan_paths.cmake")
find_package(benchmark REQUIRED)
add_dependencies(${PROJECT_NAME} benchmark::benchmark)


target_include_directories(${PROJECT_NAME} PRIVATE "Source")

target_link_libraries(${PROJECT_NAME} PRIVATE "${BENCHMARK_CPP_LINKER_FLAGS}")
target_link_libraries(${PROJECT_NAME} PRIVATE MemoryManager)
target_link_libraries(${PROJECT_NAME} PRIVATE benchmark::benchmark)

