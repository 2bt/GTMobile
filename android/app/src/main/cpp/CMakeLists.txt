cmake_minimum_required(VERSION 3.22.1)

project(native)

add_library(
    native
    SHARED
    native-lib.cpp
    src/app.cpp
    src/gtplayer.cpp
    src/gtsong.cpp
    src/sidengine.cpp
)

target_include_directories(
    native
    PUBLIC
    src
)

target_compile_options(
    native
    PRIVATE
    -O2
    -Wall
)

# disable -Wall for resid code
set_source_files_properties(
    src/sidengine.cpp
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
    oboe::oboe
    log
    android
)