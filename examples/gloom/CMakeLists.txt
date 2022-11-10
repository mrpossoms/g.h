find_package(OpenGL REQUIRED)
find_package(Ogg REQUIRED)
find_package(glew REQUIRED)

link_libraries(g)
link_libraries(glfw)
link_libraries(${OPENAL_LIBRARY})
link_libraries(vorbis)

set(GLOOM_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(FLATC ${CMAKE_SOURCE_DIR}/gitman_sources/flatbuffers/build/Release/flatc)

include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../inc ${CMAKE_CURRENT_SOURCE_DIR}/inc ${OPENAL_INCLUDE_DIR})

# set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
# set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
# set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# # add_subdirectory(../gitman_sources/glfw)

add_executable(gloom main.cpp)

add_custom_command(TARGET gloom PRE_BUILD
                   COMMAND ${FLATC} --cpp -o ${GLOOM_ROOT}/inc/api ${GLOOM_ROOT}/flat/commands/player.fbs
                  )

add_custom_command(TARGET gloom PRE_BUILD
                   COMMAND ${FLATC} --cpp -o ${GLOOM_ROOT}/inc/api ${GLOOM_ROOT}/flat/state/world.fbs
                  )

if (WIN32)
message(STATUS "NOTE: Windows requires elevated permissions to create symlinks. Please run visual studio as an administrator.")
add_custom_command(TARGET gloom   PRE_BUILD
                   COMMAND cmake -E create_symlink
                   ${CMAKE_SOURCE_DIR}/examples/gloom/data
                   $<TARGET_FILE_DIR:gloom>/../data
                  )
endif ()

add_custom_command(TARGET gloom PRE_BUILD
                   COMMAND cmake -E create_symlink
                   ${CMAKE_SOURCE_DIR}/examples/gloom/data
                   $<TARGET_FILE_DIR:gloom>/data
                  )
