cmake_minimum_required(VERSION 3.10)

project(g VERSION 0.0.1 LANGUAGES CXX)

set(DEFAULT_BUILD_TYPE "Debug")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add all source files
set(SOURCE ${SOURCE} src/g.assets.cpp)
set(SOURCE ${SOURCE} src/g.gfx.cpp)
set(SOURCE ${SOURCE} src/g.core.cpp)
set(SOURCE ${SOURCE} src/g.utils.cpp)
# set(SOURCE ${SOURCE} gitman_sources/png/*.o))
add_library(${PROJECT_NAME} STATIC ${SOURCE})

# include directories for deps
include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/xmath.h/inc)
include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/opengametools/src)

# specify includes for libg
target_include_directories(${PROJECT_NAME} PRIVATE inc)

# find_package(libpng REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

add_subdirectory(gitman_sources/png)

target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} OpenGL::GL)

#target_include_directories(${PROJECT_NAME} PUBLIC
#    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
#    $<INSTALL_INTERFACE:inc>
#    PRIVATE src)
