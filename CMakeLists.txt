cmake_minimum_required(VERSION 3.16)

project(MemoryManager)

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} 
"Source/Main.cpp")

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(${PROJECT_NAME} PRIVATE "${CPP_LINKER_FLAGS}")

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
      _CRT_SECURE_NO_WARNINGS
)

target_include_directories(${PROJECT_NAME} PUBLIC "Source")
target_include_directories(${PROJECT_NAME} PUBLIC "Vendor")