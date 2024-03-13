add_library(renderer STATIC 
    src/renderer.cpp
    src/common/BufferPool_VariableBlock.hpp
    src/common/GeometryBuffer.hpp
    src/common/RenderPass.hpp
    src/common/StagingBuffer.hpp
    src/common/loader_shared.hpp
    src/common/loader_renderpass.hpp
    src/common/loader_sortbin.hpp
    src/common/vk_enum_to_string.hpp)

message(STATUS ${vk_core_INCLUDE_DIRS})

target_include_directories(renderer PRIVATE 
    $ENV{VULKAN_SDK}/include 
    ${CMAKE_CURRENT_SOURCE_DIR}/third-party)

target_include_directories(renderer PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${vk_core_INCLUDE_DIRS}
    vk_core_INCLUDE_DIRS)

target_link_libraries(renderer PRIVATE 
    $ENV{VULKAN_SDK}/lib/libvulkan.so
    vk_core)