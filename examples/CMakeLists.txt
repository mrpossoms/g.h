find_package(OpenGL REQUIRED)
find_package(Ogg REQUIRED)

link_libraries(g)
link_libraries(glfw)
link_libraries(${OPENAL_LIBRARY})
link_libraries(vorbis)

include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../inc ${OPENAL_INCLUDE_DIR})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_executable(01.core 01.core.cpp)
add_executable(02.net_host 02.net_host.cpp)
add_executable(03.net_client 03.net_client.cpp)
add_executable(04.basic_draw 04.basic_draw.cpp)
add_executable(0425.sound 0425.sound.cpp)
add_executable(045.basic_model 045.basic_model.cpp)
add_executable(046.heightmap 046.heightmap.cpp)
add_executable(0465.sdf 0465.sdf.cpp)
add_executable(05.offscreen_render 05.offscreen_render.cpp)
add_executable(06.simple_multiplayer_game 06.simple_multiplayer_game.cpp)
add_executable(07.voxels 07.voxels.cpp)
add_executable(08.ray_march 08.ray_march.cpp)
add_executable(09.gui 09.gui.cpp)
add_executable(10.text 10.text.cpp)
add_executable(11.genetic_agent 11.genetic_agent.cpp)
add_executable(115.genetic_flight_controller 115.genetic_flight_controller.cpp)
add_executable(12.game_objects 12.game_objects.cpp)

if (WIN32)
message(STATUS "NOTE: Windows requires elevated permissions to create symlinks. Please run visual studio as an administrator.")
add_custom_command(TARGET 01.core PRE_BUILD
                   COMMAND cmake -E create_symlink
                   ${CMAKE_SOURCE_DIR}/examples/data
                   $<TARGET_FILE_DIR:01.core>/../data
                  )
endif ()

add_custom_command(TARGET 01.core PRE_BUILD
                   COMMAND cmake -E create_symlink
                   ${CMAKE_SOURCE_DIR}/examples/data
                   $<TARGET_FILE_DIR:01.core>/data
                  )
