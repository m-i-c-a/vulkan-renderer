add_executable(triangle main.cpp)

target_include_directories(triangle PRIVATE 
    glfw_INCLUDE_DIRS
    vk_core_INCLUDE_DIRS
    renderer_INCLUDE_DIRS)

target_link_libraries(triangle PRIVATE 
    glfw
    vk_core
    renderer)

install(TARGETS triangle
    RUNTIME DESTINATION ${CMAKE_HOME_DIRECTORY}/bin)