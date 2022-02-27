# ===============================================
# BUILD SYSTEM

# This is the root CMakeLists.txt that set ups
# the general environment.
# ===============================================

cmake_minimum_required(VERSION 3.16)

project(Memarena)

option(MEMORY_MANAGER_BUILD_TEST "Build the tests of the Memarena library." ON)
option(MEMORY_MANAGER_BUILD_BENCHMARKS "Build the benchmarks of the Memarena library." ON)
# option(MEMORY_MANAGER_BUILD_EXAMPLE "Build the example project that showcases how to use this library." ON)

set(CMAKE_CXX_STANDARD 20)


add_library(${PROJECT_NAME} STATIC
"Source/StackAllocator/StackAllocatorSafe.cpp"
"Source/StackAllocator/StackAllocator.cpp"
"Source/StackAllocator/StackAllocatorBase.cpp"
"Source/MemoryManager.cpp"
"Source/Utility/Alignment.cpp"
)

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_precompile_headers(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/PCH.hpp")

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
      _CRT_SECURE_NO_WARNINGS
      GLFW_INCLUDE_NONE
    PUBLIC
      $<$<CONFIG:Debug>:MEMORY_MANAGER_DEBUG>
      $<$<CONFIG:RelWithDebInfo>:MEMORY_MANAGER_DEBUG>
      $<$<CONFIG:Release>:MEMORY_MANAGER_RELEASE>
      $<$<CONFIG:MinSizeRel>:MEMORY_MANAGER_RELEASE>
)

target_include_directories(${PROJECT_NAME} INTERFACE 
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Include>
    $<INSTALL_INTERFACE:Include>
)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Include/Memarena")
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Vendor")

target_link_libraries(${PROJECT_NAME} PRIVATE "${CPP_LINKER_FLAGS}")

if (MEMORY_MANAGER_BUILD_TEST OR MEMORY_MANAGER_BUILD_BENCHMARKS)
  enable_testing()
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Tests")
endif()

if(MEMORY_MANAGER_BUILD_BENCHMARKS)
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Benchmarks")
endif()

# if (MEMORY_MANAGER_BUILD_EXAMPLE)
#   add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Vendor/benchmark")
# endif()
