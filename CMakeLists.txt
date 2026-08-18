cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(gtmobile)

include(cmake/EmbedPlayer.cmake)
write_player_embed(
    "${CMAKE_BINARY_DIR}/player_embed.hpp"
    "${CMAKE_SOURCE_DIR}/src/player.asm"
    "${CMAKE_SOURCE_DIR}/src/altplayer.asm"
)

file(GLOB ASM64_SOURCES ${CMAKE_SOURCE_DIR}/third_party/asm64/src/*.c)
list(FILTER ASM64_SOURCES EXCLUDE REGEX "/main\\.c$")
add_library(asm64 STATIC ${ASM64_SOURCES})
target_include_directories(
    asm64
    PUBLIC ${CMAKE_SOURCE_DIR}/third_party/asm64/include
)
target_compile_options(asm64 PRIVATE -std=c99 -Wall)

add_executable(
    gtmobile
    src/app.cpp
    src/app.hpp
    src/command_edit.cpp
    src/command_edit.hpp
    src/gfx.cpp
    src/gfx.hpp
    src/gtplayer.cpp
    src/gtplayer.hpp
    src/gtsong.cpp
    src/gtsong.hpp
    src/gui.cpp
    src/gui.hpp
    src/instrument_manager_view.cpp
    src/instrument_manager_view.hpp
    src/instrument_view.cpp
    src/instrument_view.hpp
    src/log.hpp
    src/piano.cpp
    src/piano.hpp
    src/platform.cpp
    src/platform.hpp
    src/project_view.cpp
    src/project_view.hpp
    src/settings_view.cpp
    src/settings_view.hpp
    src/sid.cpp
    src/sid.hpp
    src/sid_export.cpp
    src/sid_export.hpp
    src/song_view.cpp
    src/song_view.hpp
    src/song_undo.cpp
    src/song_undo.hpp
    src/vec.hpp
)

target_compile_features(gtmobile PRIVATE cxx_std_17)
target_compile_options(gtmobile PRIVATE -O2 -Wall)

set_source_files_properties(
    src/sid.cpp
    PROPERTIES COMPILE_FLAGS "-Wno-parentheses -Ofast"
)
set_source_files_properties(
    src/gfx.cpp
    PROPERTIES COMPILE_FLAGS "-Wno-unused-function"
)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLEW REQUIRED glew)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SNDFILE REQUIRED sndfile)

target_include_directories(
    gtmobile
    PRIVATE
        ${CMAKE_BINARY_DIR}
        ${GLEW_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
        ${SNDFILE_INCLUDE_DIRS}
)

target_link_libraries(
    gtmobile
    PRIVATE
        asm64
        ${GLEW_LIBRARIES}
        ${SDL2_LIBRARIES}
        ${SNDFILE_LIBRARIES}
        pthread
        -lportmidi
)

add_executable(sng2sid src/sng2sid.cpp src/gtsong.cpp src/sid_export.cpp)
target_compile_features(sng2sid PRIVATE cxx_std_17)
target_compile_options(sng2sid PRIVATE -Wall)
target_include_directories(
    sng2sid
    PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}
)
target_link_libraries(sng2sid PRIVATE asm64)

enable_testing()

set(SNG2SID_TESTDIR "${CMAKE_SOURCE_DIR}/tests")

add_executable(sng2sid_player_tests tests/player/test_player.cpp)
target_compile_features(sng2sid_player_tests PRIVATE cxx_std_17)
target_compile_options(sng2sid_player_tests PRIVATE -Wall)
target_compile_definitions(
    sng2sid_player_tests
    PRIVATE SNG2SID_DATADIR="${SNG2SID_TESTDIR}"
)
target_include_directories(sng2sid_player_tests PRIVATE ${CMAKE_BINARY_DIR})
target_link_libraries(sng2sid_player_tests PRIVATE asm64)

add_executable(
    sng2sid_pack_tests
    tests/test_pack.cpp
    src/sid_export.cpp
    src/gtsong.cpp
)
target_compile_features(sng2sid_pack_tests PRIVATE cxx_std_17)
target_compile_options(sng2sid_pack_tests PRIVATE -Wall)
target_include_directories(
    sng2sid_pack_tests
    PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}
)
target_link_libraries(sng2sid_pack_tests PRIVATE asm64)

add_test(NAME sng2sid_player_tests COMMAND sng2sid_player_tests)
add_test(NAME sng2sid_pack_tests COMMAND sng2sid_pack_tests)
add_test(
    NAME sng2sid_cli
    COMMAND
        sng2sid ${SNG2SID_TESTDIR}/fixture.sng ${CMAKE_BINARY_DIR}/fixture.sid
)
