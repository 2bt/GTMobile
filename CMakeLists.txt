cmake_minimum_required(VERSION 3.22.1)

project(gtmobile)

add_executable(
    gtmobile
    src/platform.cpp
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

target_compile_options(
    gtmobile
    PRIVATE
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

