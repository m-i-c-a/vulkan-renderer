add_executable(shadows_00 main.cpp)

target_include_directories(shadows_00 PRIVATE 
    glfw_INCLUDE_DIRS
    vk_core_INCLUDE_DIRS
    renderer_INCLUDE_DIRS)

target_link_libraries(shadows_00 PRIVATE 
    glfw
    vk_core
    renderer)

install(TARGETS shadows_00
    RUNTIME DESTINATION ${CMAKE_HOME_DIRECTORY}/bin)