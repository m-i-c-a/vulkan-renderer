add_executable(camera main.cpp)

target_include_directories(camera PRIVATE 
    glfw_INCLUDE_DIRS
    vk_core_INCLUDE_DIRS
    renderer_INCLUDE_DIRS)

target_link_libraries(camera PRIVATE 
    glfw
    vk_core
    renderer)

install(TARGETS camera
    RUNTIME DESTINATION ${CMAKE_HOME_DIRECTORY}/bin)