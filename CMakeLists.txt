cmake_minimum_required(VERSION 3.14)
project(ECS VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
add_library(${PROJECT_NAME} INTERFACE)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_17)
target_compile_definitions(${PROJECT_NAME} INTERFACE $<$<BOOL:${ECS_ENABLE_DEBUG}>:ECS_DEBUG>)
target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

if(ECS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(ECS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
