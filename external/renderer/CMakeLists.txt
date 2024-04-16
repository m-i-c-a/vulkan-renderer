add_library(renderer STATIC 
    src/renderer.cpp
    src/GlobalState.cpp src/GlobalState.hpp
    src/RenderPass.cpp src/RenderPass.hpp
    src/internal/buffers/BufferPool_VariableBlock.cpp src/internal/buffers/BufferPool_VariableBlock.hpp
    src/internal/buffers/GeometryBuffer.cpp src/internal/buffers/GeometryBuffer.hpp
    src/internal/buffers/StagingBuffer.cpp src/internal/buffers/StagingBuffer.hpp
    src/internal/buffers/UniformBuffer.cpp src/internal/buffers/UniformBuffer.hpp)

message(STATUS ${vk_core_INCLUDE_DIRS})

target_include_directories(renderer PRIVATE 
    $ENV{VULKAN_SDK}/include 
    ${CMAKE_CURRENT_SOURCE_DIR}/third-party)

target_include_directories(renderer PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${vk_core_INCLUDE_DIRS}
    vk_core_INCLUDE_DIRS)

target_link_libraries(renderer PRIVATE 
    $ENV{VULKAN_SDK}/lib/libvulkan.so
    vk_core)


set(renderer_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include PARENT_SCOPE)