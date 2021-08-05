link_libraries(g)
link_libraries(glfw)
link_libraries(OpenGL::GL)

# include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../gitman_sources/xmath.h/inc)
include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../inc)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# add_subdirectory(../gitman_sources/glfw)
find_package(OpenGL REQUIRED)

add_executable(01.core 01.core.cpp)
add_executable(02.net_host 02.net_host.cpp)
add_executable(03.net_client 03.net_client.cpp)
add_executable(04.basic_draw 04.basic_draw.cpp)
add_executable(05.offscreen_render 05.offscreen_render.cpp)
add_executable(06.simple_multiplayer_game 06.simple_multiplayer_game.cpp)
add_executable(07.voxels 07.voxels.cpp)
add_executable(08.ray_march 08.ray_march.cpp)

add_custom_command(TARGET 01.core PRE_BUILD
                   COMMAND cmake -E create_symlink
                   ${CMAKE_SOURCE_DIR}/examples/data
                   $<TARGET_FILE_DIR:01.core>/data
                  )
