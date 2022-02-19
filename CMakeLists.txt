# ===============================================
# BUILD SYSTEM

# This is the root CMakeLists.txt that set ups
# the general environment.
# ===============================================

cmake_minimum_required(VERSION 3.16)

project(MemoryManager)

set(CMAKE_CXX_STANDARD 17)
set(CPP_LINKER_FLAGS "-loleaut32 -limm32 -lversion -lole32")


add_library(${PROJECT_NAME} STATIC
"Source/StackAllocator.cpp"
"Source/MemoryManager.cpp"
"Source/Utility/Padding.cpp"
)

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_precompile_headers(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/PCH.hpp")

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
      _CRT_SECURE_NO_WARNINGS
      GLFW_INCLUDE_NONE
    PUBLIC
      $<$<CONFIG:Debug>:DEBUG>
      $<$<CONFIG:RelWithDebInfo>:DEBUG>
      $<$<CONFIG:Release>:RELEASE>
      $<$<CONFIG:MinSizeRel>:RELEASE>
)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Source")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Include/MemoryManager")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Vendor")

target_link_libraries(${PROJECT_NAME} PRIVATE "${CPP_LINKER_FLAGS}")


enable_testing()
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Vendor/googletest")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Vendor/benchmark")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Tests")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Benchmarks")
# add_subdirectory("${CMAKE_SOURCE_DIR}/Benchmarks")

