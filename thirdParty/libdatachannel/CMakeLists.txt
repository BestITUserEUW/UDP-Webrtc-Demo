cmake_minimum_required(VERSION 3.29)

project(external-libdatachannel NONE)
include(ExternalProject)

list(APPEND CMAKE_ARGS
    "-DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}"
    "-DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
    "-DBUILD_SHARED_LIBS=OFF"
    "-DNO_EXAMPLES=ON"
    "-DNO_TESTS=ON"
    "-DNO_MEDIA=OFF"
)

if(DEFINED CMAKE_TOOLCHAIN_FILE AND NOT CMAKE_TOOLCHAIN_FILE STREQUAL "")
    list(APPEND CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
endif()

message(STATUS "Preparing project ${PROJECT_NAME} with args:")

foreach(arg ${CMAKE_ARGS})
    message(STATUS "-- ${arg}")
endforeach()

ExternalProject_add(
    ${PROJECT_NAME}
    GIT_REPOSITORY https://github.com/paullouisageneau/libdatachannel.git
    GIT_TAG v0.23.0
    CMAKE_ARGS "${CMAKE_ARGS}"
    GIT_SHALLOW ON
)