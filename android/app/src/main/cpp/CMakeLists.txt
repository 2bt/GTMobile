cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_SYSTEM_NAME Android)
set(CMAKE_ANDROID_API 29)

set(LIB_DIR "../../../../native-libs")

project(native)

get_filename_component(
    GT_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.."
    ABSOLUTE
)

include(${GT_ROOT}/cmake/EmbedPlayer.cmake)
write_player_embed(
    "${CMAKE_CURRENT_BINARY_DIR}/player_embed.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/player.asm"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/altplayer.asm"
)

file(GLOB ASM64_SOURCES ${GT_ROOT}/third_party/asm64/src/*.c)
list(FILTER ASM64_SOURCES EXCLUDE REGEX "/main\\.c$")
add_library(asm64 STATIC ${ASM64_SOURCES})
target_include_directories(asm64 PUBLIC ${GT_ROOT}/third_party/asm64/include)
target_compile_options(asm64 PRIVATE -std=c99 -Wall)

add_subdirectory(${LIB_DIR} ${CMAKE_BINARY_DIR}/native_libs_build)

find_package(oboe REQUIRED CONFIG)

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
    src/sid_export.cpp
    src/song_undo.cpp
    src/song_view.cpp
)

target_include_directories(native PRIVATE src ${CMAKE_CURRENT_BINARY_DIR})

target_compile_options(native PRIVATE -std=c++17 -O3 -Wall)

set_source_files_properties(
    src/sid.cpp
    PROPERTIES COMPILE_FLAGS "-Wno-parentheses -Ofast"
)

target_link_libraries(
    native
    PRIVATE asm64 log EGL GLESv2 oboe::oboe android sndfile ogg vorbis
)
