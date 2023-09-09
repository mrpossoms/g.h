cmake_minimum_required(VERSION 3.10)

project(g VERSION 0.0.1 LANGUAGES CXX C)

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
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# c++ flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_SILENCE_DEPRECATION")

# add all source files
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.assets.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.gfx.cpp)
set(G_SOURCE ${G_SOURCE} ${CMAKE_CURRENT_SOURCE_DIR}/src/g.gfx.api.opengl.cpp)
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

# include directories for header only deps
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/xmath.h/inc)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/opengametools/src)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/lodepng)

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

message(INFO "   ___ _      _   ___  ")
message(INFO "  / __| |    /_\\ |   \\")
message(INFO " | (_ | |__ / _ \\| |) |")
message(INFO "  \\___|____/_/ \\_\\___/ ")
                       
set(GLAD_SOURCES_DIR "${PROJECT_SOURCE_DIR}/gitman_sources/glad/") # Path to glad directory
add_subdirectory("${GLAD_SOURCES_DIR}/cmake" glad_cmake) # Path to glad cmake files
glad_add_library(glad_gl_core_33 REPRODUCIBLE API gl:core=3.3) # Specify glad settings
target_link_libraries(${PROJECT_NAME} glad_gl_core_33)

# specify includes for libg
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_include_directories(${PROJECT_NAME} PUBLIC ${OPENGL_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${OPENAL_INCLUDE_DIR})

message(INFO "  _ _ _    _   _  __  __ ")
message(INFO " | (_) |__| |_(_)/ _|/ _|")
message(INFO " | | | '_ \\  _| |  _|  _|")
message(INFO " |_|_|_.__/\\__|_|_| |_|  ")
set(tiff-tools OFF)
set(tiff-docs OFF)
set(tiff-tests OFF)
set(tiff-deprecated OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/libtiff)

message(INFO "   ___ _    _____      __")
message(INFO "  / __| |  | __\\ \\    / /")
message(INFO " | (_ | |__| _| \\ \\/\\/ / ")
message(INFO "  \\___|____|_|   \\_/\\_/  ")
                         
if(EMSCRIPTEN)
else()
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/glfw)
target_link_libraries(${PROJECT_NAME} glfw)
endif()


message(INFO "  ___            _                  ")
message(INFO " | __| _ ___ ___| |_ _  _ _ __  ___ ")
message(INFO " | _| '_/ -_) -_)  _| || | '_ \\/ -_)")
message(INFO " |_||_| \\___\\___|\\__|\\_, | .__/\\___|")
message(INFO "                     |__/|_|        ")
set(WITH_ZLIB OFF CACHE BOOL "" FORCE)
set(WITH_BZip2 OFF CACHE BOOL "" FORCE)
set(WITH_PNG OFF CACHE BOOL "" FORCE)
set(WITH_HarfBuzz OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/freetype)
# this include dir seems sus... Shouldn't be needed
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/freetype/include)
target_link_libraries(${PROJECT_NAME} freetype)


message(INFO "  ___           _    _                 _ ")
message(INFO " | _ \\__ _ _ __(_)__| |_  _ __ _ _ __ | |")
message(INFO " |   / _` | '_ \\ / _` | || / _` | '  \\| |")
message(INFO " |_|_\\__,_| .__/_\\__,_|\\_, \\__,_|_|_|_|_|")
message(INFO "          |_|          |__/              ")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/rapidyaml ryml)
target_link_libraries(${PROJECT_NAME} ryml::ryml)


message(INFO "   ___            ")
message(INFO "  / _ \\ __ _ __ _ ")
message(INFO " | (_) / _` / _` |")
message(INFO "  \\___/\\__, \\__, |")
message(INFO "       |___/|___/ ")
if(EMSCRIPTEN)
else()
set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/out/build/ogg, ${CMAKE_PREFIX_PATH})
set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/out, ${CMAKE_PREFIX_PATH})
set(CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/out/lib/cmake, ${CMAKE_PREFIX_PATH})

set(INSTALL_DOCS OFF CACHE BOOL "" FORCE)
set(INSTALL_PKG_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

execute_process(COMMAND cmake -H${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/ogg/
                   -B${CMAKE_BINARY_DIR}/out/build/ogg
                   -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/out
                  )
execute_process(COMMAND cmake --build ${CMAKE_BINARY_DIR}/out/build/ogg --target install --config Debug)
execute_process(COMMAND cmake --build ${CMAKE_BINARY_DIR}/out/build/ogg --target install --config Release)
set(CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/out/lib/cmake;${CMAKE_MODULE_PATH}")
endif()


message(INFO " __   __       _    _    ")
message(INFO " \\ \\ / /__ _ _| |__(_)___")
message(INFO "  \\ V / _ \\ '_| '_ \\ (_-<")
message(INFO "   \\_/\\___/_| |_.__/_/__/")
                         
# set(OGG_LIBRARY ${CMAKE_BINARY_DIR}/out/lib/)
find_package(Ogg)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/vorbis)
target_link_libraries(${PROJECT_NAME} vorbisfile)
target_link_libraries(${PROJECT_NAME} vorbis)


message(INFO "    _          _ _     ___ _ _     ")
message(INFO "   /_\\ _  _ __| (_)___| __(_) |___ ")
message(INFO "  / _ \\ || / _` | / _ \\ _|| | / -_)")
message(INFO " /_/ \\_\\_,_\\__,_|_\\___/_| |_|_\\___|")
                                   
set(BUILD_TESTS OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/AudioFile)
target_link_libraries(${PROJECT_NAME} AudioFile)

# Windows specific linking
if (WIN32)
target_link_libraries(${PROJECT_NAME} wsock32 ws2_32)
endif()

message(INFO "============ JSON ===========")
set(BUILD_TESTS OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/gitman_sources/json)
target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)

if (G_BUILD_EXAMPLES)
message(INFO "   ___   ___                     _        ")
message(INFO "  / __| | __|_ ____ _ _ __  _ __| |___ ___")
message(INFO " | (_ | | _|\\ \\ / _` | '  \\| '_ \\ / -_|_-<")
message(INFO "  \\___| |___/_\\_\\__,_|_|_|_| .__/_\\___/__/")
message(INFO "                           |_|            ")
add_subdirectory(examples)
endif()


if (G_BUILD_TESTS)
message(INFO "   ___   _____       _      ")
message(INFO "  / __| |_   _|__ __| |_ ___")
message(INFO " | (_ |   | |/ -_|_-<  _(_-<")
message(INFO "  \\___|   |_|\\___/__/\\__/__/")
                            
add_subdirectory(tests)
endif()
