cmake_minimum_required(VERSION 3.22.1)

project(gtmobile)

add_executable(
    gtmobile
    src/app.cpp
    src/app.hpp
    src/gfx.cpp
    src/gfx.hpp
    src/gtplayer.cpp
    src/gtplayer.hpp
    src/gtsong.cpp
    src/gtsong.hpp
    src/gui.cpp
    src/gui.hpp
    src/instrument_view.cpp
    src/instrument_view.hpp
    src/log.hpp
    src/piano.cpp
    src/piano.hpp
    src/platform.cpp
    src/platform.hpp
    src/project_view.cpp
    src/project_view.hpp
    src/resid
    src/sid.cpp
    src/sid.hpp
    src/song_view.cpp
    src/song_view.hpp
    src/stb_image.h
    src/vec.hpp
    src/settings_view.cpp
    src/settings_view.hpp
)

target_compile_options(
    gtmobile
    PRIVATE
    -O2
    -Wall
)

set_source_files_properties(
    src/sid.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-parentheses -Ofast"
)
set_source_files_properties(
    src/gfx.cpp
    PROPERTIES
    COMPILE_FLAGS
    "-Wno-unused-function"
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

