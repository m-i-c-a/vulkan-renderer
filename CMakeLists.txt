cmake_minimum_required(VERSION 3.0)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)

message(STATUS "Using " $ENV{VULKAN_SDK})

add_subdirectory(vk_core)
add_subdirectory(renderer)

find_package(glfw3 REQUIRED FATAL_ERROR)

add_executable(app main.cpp)

target_include_directories(app PRIVATE 
    glfw_INCLUDE_DIRS)

target_link_libraries(app PRIVATE 
    glfw
    vk_core
    renderer)


