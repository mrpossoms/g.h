enable_testing()

find_package(OpenGL REQUIRED)
find_package(Ogg REQUIRED)
find_package(glew REQUIRED)

link_libraries(g)
link_libraries(glfw)
link_libraries(${OPENAL_LIBRARY})
# link_libraries(ogg)
link_libraries(vorbis)

# include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../gitman_sources/xmath.h/inc)
include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../inc)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# add_subdirectory(../gitman_sources/glfw)

add_executable(voxel-hash voxel-hash.cpp)
add_executable(vox-scene vox-scene.cpp)
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

add_test(NAME voxel-hash COMMAND voxel-hash)
add_test(NAME vox-scene COMMAND vox-scene)
add_test(NAME ray-plane-intersect COMMAND ray-plane-intersect)
add_test(NAME thread-pool COMMAND thread-pool)
add_test(NAME game-object COMMAND game-object)
