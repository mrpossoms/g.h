cmake_minimum_required(VERSION 3.13)

project(g VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -Wall -g -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Wall -g -O0")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -Wall -O3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Wall -O3")

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
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# c++ flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_SILENCE_DEPRECATION")

# add all source files
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.assets.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.gfx.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.gfx.mesh_factory.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.gfx.primative.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.gfx.debug.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.snd.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.snd.track_factory.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.core.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.utils.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.ui.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/lodepng/lodepng.cpp)

# set(SOURCE ${SOURCE} gitman_sources/png/*.o))
add_library(${PROJECT_NAME} STATIC ${G_SOURCE})

execute_process(COMMAND cmake -H${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/glew/build/cmake
                   -B${CMAKE_BINARY_DIR}/out/build/glew
                   -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/out
                  )
execute_process(COMMAND cmake --build out/build/glew --target install --config Debug)
execute_process(COMMAND cmake --build out/build/glew --target install --config Release)

if (WIN32)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif ()

message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
set(GLEW_USE_STATIC_LIBS 1)
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/out)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/out)

# include directories for deps
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/xmath.h/inc)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/opengametools/src)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/lodepng)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/glfw/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/glew/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/freetype/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/vorbis/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/ogg/include)

# specify includes for libg
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_include_directories(${PROJECT_NAME} PUBLIC ${OPENGL_INCLUDE_DIR})

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/glfw)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/freetype)

# set(GLEW_DIR /home/kirk/code/g.h/gitman_sources/glew/build/cmake/install/lib/cmake/glew)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/glew/build/cmake)
find_package(GLEW REQUIRED CONFIG) # call twice to test multiple call
find_package(OpenGL REQUIRED)

find_package(OpenAL REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${OPENAL_INCLUDE_DIR})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/AudioFile)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/vorbis)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/ogg)

# target_link_libraries(${PROJECT_NAME} PRIVATE GLEW::GLEW ${OPENGL_LIBRARIES})
target_link_libraries(${PROJECT_NAME} glew_s)
target_link_libraries(${PROJECT_NAME} freetype)
target_link_libraries(${PROJECT_NAME} OpenGL::GL)
target_link_libraries(${PROJECT_NAME} ogg)
target_link_libraries(${PROJECT_NAME} vorbis)
target_link_libraries(${PROJECT_NAME} vorbisfile)
target_link_libraries(${PROJECT_NAME} AudioFile)

add_subdirectory(examples)
add_subdirectory(tests)
