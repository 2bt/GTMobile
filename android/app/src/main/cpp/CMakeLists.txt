cmake_minimum_required(VERSION 3.22.1)

project(native)

add_library(
    native
    SHARED
    native-lib.cpp
    src/gtplayer.cpp
    src/gtsong.cpp
    src/sid.cpp
    src/gfx.cpp
    src/gui.cpp
    src/app.cpp
    src/piano.cpp
    src/project_view.cpp
    src/song_view.cpp
    src/instrument_view.cpp
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

# disable -Wall for resid code
set_source_files_properties(
    src/sid.cpp
    PROPERTIES
    COMPILE_OPTIONS
    -Wno-all
)

find_package (
    oboe
    REQUIRED
    CONFIG
)

target_link_libraries(
    native
    log
    EGL
    GLESv2
    oboe::oboe
    android
)
