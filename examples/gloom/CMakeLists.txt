find_package(OpenGL REQUIRED)
find_package(Ogg REQUIRED)
find_package(glew REQUIRED)

link_libraries(g)
link_libraries(glfw)
link_libraries(${OPENAL_LIBRARY})
link_libraries(vorbis)

set(GLOOM_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

if (WIN32)
set(FLATC ${CMAKE_SOURCE_DIR}/gitman_sources/flatbuffers/build/Release/flatc)
else()
set(FLATC ${CMAKE_SOURCE_DIR}/gitman_sources/flatbuffers/build/flatc)
endif()

include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc 
    ${CMAKE_CURRENT_SOURCE_DIR}/inc 
    ${OPENAL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/gitman_sources/flatbuffers/include
    )

message("Using flatc found at '${FLATC}'")

add_custom_command(
    OUTPUT ${GLOOM_ROOT}/inc/api/api_generated.h
    COMMAND ${FLATC} --cpp --gen-all -o ${GLOOM_ROOT}/inc/api ${GLOOM_ROOT}/flat/api.fbs
    DEPENDS ${GLOOM_ROOT}/flat/api.fbs ${FLATC} main.cpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

add_executable(gloom main.cpp ${GLOOM_ROOT}/inc/api/api_generated.h)

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
