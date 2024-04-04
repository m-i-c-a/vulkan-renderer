cmake_minimum_required(VERSION 3.0)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)

message(STATUS "Using " $ENV{VULKAN_SDK})

find_package(glfw3 REQUIRED FATAL_ERROR)

add_subdirectory(external)
add_subdirectory(examples)