# ===================================================
# BUILD SYSTEM

# This is the CMakeLists.txt that generates the
# executable for the Tests
# ===================================================

project(Tests)

add_executable(${PROJECT_NAME} 
"Source/Main.cpp"
"Source/StackAllocatorTest.cpp"
"Source/MemoryManagerTest.cpp"
)

target_include_directories(${PROJECT_NAME} PUBLIC "Source")
target_include_directories(${PROJECT_NAME} PUBLIC "../Include")

target_link_libraries(${PROJECT_NAME} PRIVATE gtest_main)
target_link_libraries(${PROJECT_NAME} PRIVATE MemoryManager)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
      _CRT_SECURE_NO_WARNINGS
    PUBLIC
      $<$<CONFIG:Debug>:MEMORY_MANAGER_DEBUG>
      $<$<CONFIG:RelWithDebInfo>:MEMORY_MANAGER_DEBUG>
      $<$<CONFIG:Release>:MEMORY_MANAGER_RELEASE>
      $<$<CONFIG:MinSizeRel>:MEMORY_MANAGER_RELEASE>
)

# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/Catch2/extras")

# include(CTest)
# include(Catch)

# catch_discover_tests(${PROJECT_NAME})

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})