cmake_minimum_required(VERSION 3.10)

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
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# c++ flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_SILENCE_DEPRECATION")

# add all source files
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.assets.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.gfx.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.gfx.mesh_factory.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.gfx.primative.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.gfx.debug.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.gfx.effect.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.snd.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.snd.track_factory.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.core.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.utils.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.ui.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.io.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/lodepng/lodepng.cpp)

add_library(${PROJECT_NAME} STATIC ${G_SOURCE})

if (WIN32)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/out)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/out)

# include directories for deps
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/xmath.h/inc)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/opengametools/src)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/lodepng)

if(EMSCRIPTEN)
else()
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/glfw/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/glew/include)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/freetype/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/vorbis/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/ogg/include)

# specify includes for libg
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_include_directories(${PROJECT_NAME} PUBLIC ${OPENGL_INCLUDE_DIR})

if(EMSCRIPTEN)
else()
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/glfw)
endif()

set(WITH_ZLIB OFF CACHE BOOL "" FORCE)
set(WITH_BZip2 OFF CACHE BOOL "" FORCE)
set(WITH_PNG OFF CACHE BOOL "" FORCE)
set(WITH_HarfBuzz OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/freetype)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/rapidyaml ryml)

# set(GLEW_DIR /home/kirk/code/g.h/gitman_sources/glew/build/cmake/install/lib/cmake/glew)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/glew/build/cmake)

# build and use ogg
execute_process(COMMAND cmake -H${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/ogg/
                   -B${CMAKE_BINARY_DIR}/out/build/ogg
                   -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/out
                  )
execute_process(COMMAND cmake --build ${CMAKE_BINARY_DIR}/out/build/ogg --target install --config Debug)
execute_process(COMMAND cmake --build ${CMAKE_BINARY_DIR}/out/build/ogg --target install --config Release)
set(OGG_LIBRARY ${CMAKE_BINARY_DIR}/out/lib/libogg.a)
set(OGG_INCLUDE_DIR ${CMAKE_BINARY_DIR}/out/include)

if(EMSCRIPTEN)
else()
# build and use glew
execute_process(COMMAND cmake -H${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/glew/build/cmake
                   -B${CMAKE_BINARY_DIR}/out/build/glew
                   -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/out
                  )
execute_process(COMMAND cmake --build out/build/glew --target install --config Debug)
execute_process(COMMAND cmake --build out/build/glew --target install --config Release)

set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/out/build/glew, ${CMAKE_PREFIX_PATH})
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/glew/build/cmake)
# find_package(GLEW REQUIRED CONFIG)


set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/out/build/ogg, ${CMAKE_PREFIX_PATH})
set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/out, ${CMAKE_PREFIX_PATH})
set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/out/lib/cmake, ${CMAKE_PREFIX_PATH})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/ogg)
#find_package(Ogg REQUIRED CONFIG)
endif()

# find OpenGL
find_package(OpenGL REQUIRED)

# find OpenAL
if(WIN32)
set(OPENAL_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/external/windows/openal/libs/Win64/OpenAL32.lib")
set(OPENAL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/windows/openal/include")
set(OPENAL_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/external/windows/openal/libs/Win64/OpenAL32.lib" PARENT_SCOPE)
set(OPENAL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/windows/openal/include" PARENT_SCOPE)
message("Using static OPENAL_LIBRARY: ${OPENAL_LIBRARY}")
else()
find_package(OpenAL REQUIRED)
endif ()

target_include_directories(${PROJECT_NAME} PUBLIC ${OPENAL_INCLUDE_DIR})

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/ogg)
# set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/ogg)

# set(OGG_LIBRARY ${CMAKE_BINARY_DIR}/out/lib/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/vorbis)
# set(CMAKE_PREFIX_PATH "")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/AudioFile)

if(EMSCRIPTEN)
else()
target_link_libraries(${PROJECT_NAME} glew_s)
endif()

target_link_libraries(${PROJECT_NAME} freetype)
target_link_libraries(${PROJECT_NAME} vorbis)
target_link_libraries(${PROJECT_NAME} vorbisfile)
target_link_libraries(${PROJECT_NAME} AudioFile)
target_link_libraries(${PROJECT_NAME} ryml::ryml)


if (G_BUILD_EXAMPLES)
add_subdirectory(examples)
endif()

if (G_BUILD_TESTS)
add_subdirectory(tests)
endif()
