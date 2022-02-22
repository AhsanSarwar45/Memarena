# ===================================================
# BUILD SYSTEM

# This is the CMakeLists.txt that generates the
# executable for the Tests
# ===================================================

project(Tests)

conan_cmake_configure(REQUIRES gtest/1.10.0
                      OPTIONS gtest:build_gmock=False
                      GENERATORS cmake_find_package cmake_paths)

conan_cmake_autodetect(settings)

conan_cmake_install(PATH_OR_REFERENCE .
                    BUILD missing
                    REMOTE conancenter
                    SETTINGS ${settings})

add_executable(${PROJECT_NAME} 
"Source/Main.cpp"
"Source/StackAllocatorTest.cpp"
"Source/MemoryManagerTest.cpp"
)

target_include_directories(${PROJECT_NAME} PRIVATE "Source")

include("${CMAKE_BINARY_DIR}/${PROJECT_NAME}/conan_paths.cmake")
find_package(GTest REQUIRED)
add_dependencies(${PROJECT_NAME} GTest::gtest)

target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest)
target_link_libraries(${PROJECT_NAME} PRIVATE MemoryManager)

# target_compile_definitions(${PROJECT_NAME}
#     PUBLIC
#       $<$<CONFIG:Debug>:MEMORY_MANAGER_DEBUG>
#       $<$<CONFIG:RelWithDebInfo>:MEMORY_MANAGER_DEBUG>
#       $<$<CONFIG:Release>:MEMORY_MANAGER_RELEASE>
#       $<$<CONFIG:MinSizeRel>:MEMORY_MANAGER_RELEASE>
# )

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})