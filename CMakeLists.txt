# ===============================================
# BUILD SYSTEM

# This is the root CMakeLists.txt that set ups
# the general environment.
# ===============================================

cmake_minimum_required(VERSION 3.16)

project(Memarena)

option(MEMARENA_BUILD_TEST "Build the tests of the Memarena library." ON)
option(MEMARENA_BUILD_BENCHMARKS "Build the benchmarks of the Memarena library." ON)
option(MEMARENA_CPPCHECK "Run the cppcheck static analyzer." ON)
# option(MEMARENA_BUILD_EXAMPLE "Build the example project that showcases how to use this library." ON)

set(CMAKE_CXX_STANDARD 20)

if (MEMARENA_CPPCHECK)
set(CMAKE_CXX_CPPCHECK "cppcheck")
endif()

add_library(${PROJECT_NAME} STATIC
"Source/Allocator.cpp"
"Source/AllocatorUtils.cpp"
"Source/MemoryManager.cpp"
"Source/Utility/Alignment.cpp"
)

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_precompile_headers(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/PCH.hpp")

target_compile_definitions(${PROJECT_NAME}
    # PRIVATE
      # _CRT_SECURE_NO_WARNINGS
    PUBLIC
      $<$<CONFIG:Debug>:MEMARENA_DEBUG>
      $<$<CONFIG:RelWithDebInfo>:MEMARENA_DEBUG>
      $<$<CONFIG:Release>:MEMARENA_RELEASE>
      $<$<CONFIG:MinSizeRel>:MEMARENA_RELEASE>
)

target_include_directories(${PROJECT_NAME} INTERFACE 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Include>
    $<INSTALL_INTERFACE:Include>
)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
# target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Include/Memarena")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Vendor")

target_link_libraries(${PROJECT_NAME} PRIVATE "${CPP_LINKER_FLAGS}")

if (MEMARENA_BUILD_TEST OR MEMARENA_BUILD_BENCHMARKS)
  enable_testing()
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Tests")
endif()

if(MEMARENA_BUILD_BENCHMARKS)
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Benchmarks")
endif()

# if (MEMARENA_BUILD_EXAMPLE)
#   add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Vendor/benchmark")
# endif()
