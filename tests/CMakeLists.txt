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
