cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_ANDROID_API 29)

set(LIB_DIR "../../../../native-libs")

project(native)

add_subdirectory(${LIB_DIR}/libsndfile ${CMAKE_BINARY_DIR}/libsndfile_build)

find_package (
    oboe
    REQUIRED
    CONFIG
)

add_library(
    native
    SHARED
    native-lib.cpp
    src/app.cpp
    src/command_edit.cpp
    src/command_edit.hpp
    src/gfx.cpp
    src/gtplayer.cpp
    src/gtsong.cpp
    src/gui.cpp
    src/instrument_view.cpp
    src/instrument_manager_view.cpp
    src/piano.cpp
    src/project_view.cpp
    src/settings_view.cpp
    src/sid.cpp
    src/song_view.cpp
)

target_include_directories(
    native
    PRIVATE
    src
)

target_compile_options(
    native
    PRIVATE
    -std=c++17
    -O3
    -Wall
)

set_source_files_properties(
    src/sid.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-parentheses -Ofast"
)

target_link_libraries(
    native
    PRIVATE
    log
    EGL
    GLESv2
    oboe::oboe
    android
    sndfile
)
