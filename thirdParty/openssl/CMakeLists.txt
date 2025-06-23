cmake_minimum_required(VERSION 3.29)

project(external-openssl NONE)
include(ExternalProject)
include(ProcessorCount)
ProcessorCount(NUM_PROC)

find_program(MAKE_EXE NAMES make)

set(OPENSSL_VERSION 3.5.0)

message(STATUS "OPENSSL_VERSION set to ${OPENSSL_VERSION}")

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    set(OPENSSL_PLATFORM "linux-aarch64")
else()
    set(OPENSSL_PLATFORM "linux-x86_64")
endif()

message(STATUS "OPENSSL_PLATFORM set to ${OPENSSL_PLATFORM}")

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(OPENSSL_BUILD_TYPE "--debug")
else()
    set(OPENSSL_BUILD_TYPE "--release")
endif()

message(STATUS "OPENSSL_BUILD_TYPE set to ${OPENSSL_BUILD_TYPE}")

set(CONFIGURE_ARGS
    ${OPENSSL_PLATFORM}
    --prefix=${CMAKE_INSTALL_PREFIX}
    --openssldir=${CMAKE_INSTALL_PREFIX}
    --libdir=lib
    ${OPENSSL_BUILD_TYPE}
    no-shared
    no-dso
    no-engine
    no-tests
)

message(STATUS "Preparing project ${PROJECT_NAME} with args:")

foreach(CONFIGURE_ARG ${CONFIGURE_ARGS})
    message(STATUS "-- ${CONFIGURE_ARG}")
endforeach()

ExternalProject_add(
    ${PROJECT_NAME}
    URL https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env CROSS_COMPILE="" <SOURCE_DIR>/Configure ${CONFIGURE_ARGS}
    BUILD_COMMAND ${MAKE_EXE} -j${NUM_PROC}
    INSTALL_COMMAND ${MAKE_EXE} install_sw -j${NUM_PROC}
)