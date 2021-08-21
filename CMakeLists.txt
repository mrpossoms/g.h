cmake_minimum_required(VERSION 3.10)

project(g VERSION 0.0.1 LANGUAGES CXX)

set(DEFAULT_BUILD_TYPE "Debug")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# prefer new vendor provided GL implementations
if(POLICY CMP0072)
  cmake_policy (SET CMP0072 NEW)
  set(OpenGL_GL_PREFERENCE GLVND)
endif()

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# c++ flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_SILENCE_DEPRECATION")

# add all source files
set(SOURCE ${SOURCE} src/g.assets.cpp)
set(SOURCE ${SOURCE} src/g.gfx.cpp)
set(SOURCE ${SOURCE} src/g.core.cpp)
set(SOURCE ${SOURCE} src/g.utils.cpp)
set(SOURCE ${SOURCE} gitman_sources/lodepng/lodepng.cpp)

# set(SOURCE ${SOURCE} gitman_sources/png/*.o))
add_library(${PROJECT_NAME} STATIC ${SOURCE})

execute_process(COMMAND cmake -H../gitman_sources/glew/build/cmake 
                   -Bout/build/glew 
                   -DCMAKE_INSTALL_PREFIX=out
                  )

if (WIN32)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif ()

set(GLEW_USE_STATIC_LIBS 1)
set(CMAKE_PREFIX_PATH ./out)
set(CMAKE_INSTALL_PREFIX ./out)

# include directories for deps
include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/xmath.h/inc)
include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/opengametools/src)
include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/lodepng)
include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/glfw/include)
include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/glew/include)

# specify includes for libg
target_include_directories(${PROJECT_NAME} PRIVATE inc)
target_include_directories(${PROJECT_NAME} PRIVATE ${OPENGL_INCLUDE_DIR})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(gitman_sources/glfw)

# set(GLEW_DIR /home/kirk/code/g.h/gitman_sources/glew/build/cmake/install/lib/cmake/glew)
add_subdirectory(gitman_sources/glew/build/cmake)
find_package(GLEW REQUIRED CONFIG) # call twice to test multiple call
find_package(OpenGL REQUIRED)

# target_link_libraries(${PROJECT_NAME} PRIVATE GLEW::GLEW ${OPENGL_LIBRARIES})
target_link_libraries(${PROJECT_NAME} glew_s)
target_link_libraries(${PROJECT_NAME} OpenGL::GL)

add_subdirectory(examples)
