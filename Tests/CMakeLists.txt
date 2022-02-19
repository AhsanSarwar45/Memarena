# ===================================================
# BUILD SYSTEM

# This is the CMakeLists.txt that generates the
# executable for the Tests
# ===================================================

project(Tests)

add_executable(${PROJECT_NAME} 
"Source/Main.cpp"
"Source/StackAllocatorTest.cpp"
)

target_include_directories(${PROJECT_NAME} PUBLIC "Source")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/Include")

target_link_libraries(${PROJECT_NAME} PRIVATE gtest_main)
target_link_libraries(${PROJECT_NAME} PRIVATE MemoryManager)

# list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Vendor/Catch2/extras")

# include(CTest)
# include(Catch)

# catch_discover_tests(${PROJECT_NAME})

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})