find_package(OpenGL REQUIRED)

link_libraries(g)
link_libraries(glfw)
# link_libraries(OpenGL::GL)

# include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../gitman_sources/xmath.h/inc)
include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../inc)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# add_subdirectory(../gitman_sources/glfw)

add_executable(voxel-hash voxel-hash.cpp)
add_executable(ray-plane-intersect ray-plane-intersect.cpp)
add_executable(thread-pool thread-pool.cpp)
add_executable(game-object game-object.cpp)

if (WIN32)
message(STATUS "NOTE: Windows requires elevated permissions to create symlinks. Please run visual studio as an administrator.")
add_custom_command(TARGET game-object PRE_BUILD
                   COMMAND cmake -E create_symlink
                   ${CMAKE_SOURCE_DIR}/tests/data
                   $<TARGET_FILE_DIR:game-object>/../data
                  )
endif ()

add_custom_command(TARGET game-object PRE_BUILD
                   COMMAND cmake -E create_symlink
                   ${CMAKE_SOURCE_DIR}/tests/data
                   $<TARGET_FILE_DIR:game-object>/data
                  )