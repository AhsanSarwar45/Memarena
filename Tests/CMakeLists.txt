# ===================================================
# BUILD SYSTEM

# This is the CMakeLists.txt that generates the
# executable for the Tests
# ===================================================

project(Tests)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan.cmake"
                EXPECTED_HASH SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
                TLS_VERIFY ON)
endif()

include("${CMAKE_BINARY_DIR}/conan.cmake")

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

include("${CMAKE_CURRENT_BINARY_DIR}/conan_paths.cmake")
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