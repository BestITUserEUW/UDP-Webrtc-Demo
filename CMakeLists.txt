cmake_minimum_required(VERSION 3.10...3.29)

cmake_policy(SET CMP0146 OLD)
cmake_policy(SET CMP0169 OLD)

project(webrtc-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(DEPS_BUILD_PATH "${PROJECT_BINARY_DIR}/thirdParty" CACHE PATH "Install path for the dependencies")
set(DEPS_INSTALL_PATH "${DEPS_BUILD_PATH}/install" CACHE PATH "Install path for the dependencies")

link_directories(
    ${DEPS_INSTALL_PATH}/lib
)

add_subdirectory(thirdParty)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(LibDataChannel REQUIRED)
find_package(oryx-crt-cpp REQUIRED)

pkg_search_module(x264 REQUIRED IMPORTED_TARGET x264)
pkg_search_module(libavcodec REQUIRED IMPORTED_TARGET libavcodec)
pkg_search_module(libswscale REQUIRED IMPORTED_TARGET libswscale)
pkg_check_modules(libavutil REQUIRED IMPORTED_TARGET libavutil)


add_library(common_runtime STATIC 
    src/ffmpeg_decoder.cpp
    src/ffmpeg_encoder.cpp
    src/signal_handler.cpp
)

target_include_directories(common_runtime PUBLIC
    ${DEPS_INSTALL_PATH}/include
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(common_runtime PUBLIC
    Threads::Threads
    PkgConfig::x264
    PkgConfig::libavcodec
    PkgConfig::libswscale
    PkgConfig::libavutil
    LibDataChannel::LibDataChannel
    oryx::oryx-crt-cpp
    ${OpenCV_LIBS}
)

target_compile_options(common_runtime PUBLIC
    -Wall -Wextra -Wuninitialized -Wno-unused-function -Wunused-variable -Wno-interference-size -ftemplate-depth=2048 -fconstexpr-depth=2048 $<$<CONFIG:Release>:-g0> $<$<CONFIG:Release>:-O3>
)

target_link_options(common_runtime PUBLIC
    -static-libgcc -static-libstdc++ $<$<CONFIG:Release>:-s>
)


list(APPEND Exes receiver sender)

foreach(exe IN LISTS Exes)
    add_executable(wrtc_${exe}
        src/${exe}.cpp
    )   

    target_link_libraries(wrtc_${exe} PRIVATE
        common_runtime
    )
endforeach()
