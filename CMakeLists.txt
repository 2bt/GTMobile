cmake_minimum_required(VERSION 3.22.1)

project(gtmobile)

add_executable(
    gtmobile
    src/platform.cpp
    src/app.cpp
    src/gfx.cpp
    src/gtplayer.cpp
    src/gtsong.cpp
    src/sidengine.cpp
)

target_compile_options(
    gtmobile
    PRIVATE
    -O3
    -Wall
)

# disable -Wall for resid code
set_source_files_properties(
    src/sidengine.cpp
    PROPERTIES
    COMPILE_OPTIONS
    -Wno-all
)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLEW REQUIRED glew)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SNDFILE REQUIRED sndfile)

target_include_directories(
    gtmobile
    SYSTEM
    PUBLIC
    ${GLEW_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${SNDFILE_INCLUDE_DIRS}
)

target_link_libraries(
    gtmobile
    ${GLEW_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${SNDFILE_LIBRARIES}
    pthread
)

