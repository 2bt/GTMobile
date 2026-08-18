cmake_minimum_required(VERSION 3.22.1)

file(GLOB ASM64_SOURCES ${CMAKE_SOURCE_DIR}/third_party/asm64/src/*.c)
list(FILTER ASM64_SOURCES EXCLUDE REGEX "/main\\.c$")

add_library(asm64 STATIC ${ASM64_SOURCES})
target_include_directories(asm64 PUBLIC ${CMAKE_SOURCE_DIR}/third_party/asm64/include)
target_compile_options(asm64 PRIVATE -std=c99 -Wall)

function(sng2sid_embed_asm var file)
    file(READ "${file}" hex HEX)
    if(hex STREQUAL "")
        message(FATAL_ERROR "failed to read ${file}")
    endif()
    string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1," hex "${hex}")
    set(${var} "${hex}" PARENT_SCOPE)
endfunction()
sng2sid_embed_asm(PLAYER_ASM_BYTES "${CMAKE_CURRENT_SOURCE_DIR}/player.asm")
sng2sid_embed_asm(ALTPLAYER_ASM_BYTES "${CMAKE_CURRENT_SOURCE_DIR}/altplayer.asm")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/player_embed.hpp"
    "static char const PLAYER_ASM[] = {${PLAYER_ASM_BYTES}};\n"
    "static char const ALTPLAYER_ASM[] = {${ALTPLAYER_ASM_BYTES}};\n")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/player.asm"
    "${CMAKE_CURRENT_SOURCE_DIR}/altplayer.asm")

set(SNG2SID_DATADIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(GTSONG_SRC ${CMAKE_SOURCE_DIR}/src/gtsong.cpp)

add_executable(
    sng2sid
    main.cpp
    ${GTSONG_SRC}
    sid_export.cpp
)
target_compile_features(sng2sid PRIVATE cxx_std_17)
target_compile_options(sng2sid PRIVATE -Wall)
target_include_directories(sng2sid PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(sng2sid PRIVATE asm64)

add_executable(
    sng2sid_player_tests
    test/player/test_player.cpp
)
target_compile_features(sng2sid_player_tests PRIVATE cxx_std_17)
target_compile_options(sng2sid_player_tests PRIVATE -Wall)
target_compile_definitions(sng2sid_player_tests PRIVATE SNG2SID_DATADIR="${SNG2SID_DATADIR}")
target_include_directories(sng2sid_player_tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(sng2sid_player_tests PRIVATE asm64)

add_executable(
    sng2sid_pack_tests
    test/test_pack.cpp
    sid_export.cpp
    ${GTSONG_SRC}
)
target_compile_features(sng2sid_pack_tests PRIVATE cxx_std_17)
target_compile_options(sng2sid_pack_tests PRIVATE -Wall)
target_include_directories(sng2sid_pack_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(sng2sid_pack_tests PRIVATE asm64)

add_test(NAME sng2sid_player_tests COMMAND sng2sid_player_tests)
add_test(NAME sng2sid_pack_tests COMMAND sng2sid_pack_tests)
add_test(
    NAME sng2sid_cli
    COMMAND sng2sid ${CMAKE_CURRENT_SOURCE_DIR}/test/fixture.sng ${CMAKE_CURRENT_BINARY_DIR}/fixture.sid
)

set(GOATTRACKER_DIR "" CACHE PATH "GoatTracker source tree for regenerating player goldens")
if(GOATTRACKER_DIR)
    file(GLOB EXOMIZER_ASM_SOURCES ${GOATTRACKER_DIR}/src/asm/*.c)
    add_executable(exomizer_asm test/player/exomizer_asm.c ${EXOMIZER_ASM_SOURCES})
    target_include_directories(exomizer_asm PRIVATE ${GOATTRACKER_DIR}/src/asm)
    add_custom_target(
        sng2sid_player_goldens
        COMMAND ${CMAKE_COMMAND} -E env
            GOATTRACKER_DIR=${GOATTRACKER_DIR}
            EXOMIZER_ASM=$<TARGET_FILE:exomizer_asm>
            SNG2SID_DATADIR=${SNG2SID_DATADIR}
            python3 ${CMAKE_CURRENT_SOURCE_DIR}/test/player/gen_goldens.py
        DEPENDS exomizer_asm
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Regenerate Exomizer player goldens"
    )
endif()
