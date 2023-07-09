cmake_minimum_required(VERSION 3.22.1)

project(native)

add_library(
    native
    SHARED
    native-lib.cpp
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
)

find_package (
    oboe
    REQUIRED
    CONFIG
)

target_link_libraries(
    native
    log
    oboe::oboe
)