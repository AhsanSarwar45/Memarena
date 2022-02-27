# ===================================================
# BUILD SYSTEM

# This is the CMakeLists.txt that generates the
# executable for the Benchmarks
# ===================================================

project(Benchmarks)

set(BENCHMARK_CPP_LINKER_FLAGS "-lpthread")

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan.cmake"
                EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
                TLS_VERIFY ON)
endif()

include("${CMAKE_BINARY_DIR}/conan.cmake")


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
"Source/StackAllocatorAllocationsBenchmark.cpp"
"Source/StackAllocatorAccessBenchmark.cpp"
"Source/AlignmentBenchmark.cpp"
)

include("${CMAKE_CURRENT_BINARY_DIR}/conan_paths.cmake")
find_package(benchmark REQUIRED)
add_dependencies(${PROJECT_NAME} benchmark::benchmark)


target_include_directories(${PROJECT_NAME} PRIVATE "Source")

target_link_libraries(${PROJECT_NAME} PRIVATE "${BENCHMARK_CPP_LINKER_FLAGS}")
target_link_libraries(${PROJECT_NAME} PRIVATE Memarena)
target_link_libraries(${PROJECT_NAME} PRIVATE benchmark::benchmark)

