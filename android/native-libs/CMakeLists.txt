cmake_minimum_required(VERSION 3.22.1)

# Android build of libsndfile + libogg + libvorbis (git submodules).
# WAV and Ogg/Vorbis export only; FLAC and Opus-in-Ogg are stubbed out.

set(LIBOGG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libogg)
set(LIBVORBIS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libvorbis)
set(LIBSNDFILE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libsndfile)

#
# libogg
#

add_library(ogg STATIC ${LIBOGG_DIR}/src/bitwise.c ${LIBOGG_DIR}/src/framing.c)
target_include_directories(
    ogg
    PUBLIC
        ${LIBOGG_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR} # ogg/config_types.h (normally autotools-generated)
)

#
# libvorbis — one static library for vorbis, vorbisenc, and vorbisfile
#

add_library(
    vorbis
    STATIC
    ${LIBVORBIS_DIR}/lib/mdct.c
    ${LIBVORBIS_DIR}/lib/smallft.c
    ${LIBVORBIS_DIR}/lib/block.c
    ${LIBVORBIS_DIR}/lib/envelope.c
    ${LIBVORBIS_DIR}/lib/window.c
    ${LIBVORBIS_DIR}/lib/lsp.c
    ${LIBVORBIS_DIR}/lib/lpc.c
    ${LIBVORBIS_DIR}/lib/analysis.c
    ${LIBVORBIS_DIR}/lib/synthesis.c
    ${LIBVORBIS_DIR}/lib/psy.c
    ${LIBVORBIS_DIR}/lib/info.c
    ${LIBVORBIS_DIR}/lib/floor1.c
    ${LIBVORBIS_DIR}/lib/floor0.c
    ${LIBVORBIS_DIR}/lib/res0.c
    ${LIBVORBIS_DIR}/lib/mapping0.c
    ${LIBVORBIS_DIR}/lib/registry.c
    ${LIBVORBIS_DIR}/lib/codebook.c
    ${LIBVORBIS_DIR}/lib/sharedbook.c
    ${LIBVORBIS_DIR}/lib/lookup.c
    ${LIBVORBIS_DIR}/lib/bitrate.c
    ${LIBVORBIS_DIR}/lib/vorbisfile.c
    ${LIBVORBIS_DIR}/lib/vorbisenc.c
)

target_include_directories(vorbis PUBLIC ${LIBVORBIS_DIR}/include)
target_include_directories(
    vorbis
    PRIVATE
        ${LIBVORBIS_DIR}/lib
        ${LIBVORBIS_DIR}/lib/books/floor
        ${LIBVORBIS_DIR}/lib/books/coupled
        ${LIBVORBIS_DIR}/lib/books/uncoupled
)
target_link_libraries(vorbis PUBLIC ogg)

add_library(Ogg::ogg ALIAS ogg)
add_library(Vorbis::vorbis ALIAS vorbis)
add_library(Vorbis::vorbisenc ALIAS vorbis)
add_library(Vorbis::vorbisfile ALIAS vorbis)

#
# libsndfile — point FindOgg / FindVorbis at our static libraries
#

set(Ogg_INCLUDE_DIR ${LIBOGG_DIR}/include CACHE PATH "" FORCE)
set(Ogg_LIBRARY ogg CACHE FILEPATH "" FORCE)
set(Ogg_FOUND TRUE CACHE BOOL "" FORCE)

set(Vorbis_Vorbis_INCLUDE_DIR ${LIBVORBIS_DIR}/include CACHE PATH "" FORCE)
set(Vorbis_Enc_INCLUDE_DIR ${LIBVORBIS_DIR}/include CACHE PATH "" FORCE)
set(Vorbis_File_INCLUDE_DIR ${LIBVORBIS_DIR}/include CACHE PATH "" FORCE)
set(Vorbis_Vorbis_LIBRARY vorbis CACHE FILEPATH "" FORCE)
set(Vorbis_Enc_LIBRARY vorbis CACHE FILEPATH "" FORCE)
set(Vorbis_File_LIBRARY vorbis CACHE FILEPATH "" FORCE)
set(Vorbis_Vorbis_FOUND TRUE CACHE BOOL "" FORCE)
set(Vorbis_Enc_FOUND TRUE CACHE BOOL "" FORCE)
set(Vorbis_File_FOUND TRUE CACHE BOOL "" FORCE)
set(Vorbis_FOUND TRUE CACHE BOOL "" FORCE)

set(ENABLE_EXTERNAL_LIBS ON CACHE BOOL "" FORCE)
set(ENABLE_MPEG OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_CPACK OFF CACHE BOOL "" FORCE)
set(ENABLE_PACKAGE_CONFIG OFF CACHE BOOL "" FORCE)
set(INSTALL_PKGCONFIG_MODULE OFF CACHE BOOL "" FORCE)

add_subdirectory(${LIBSNDFILE_DIR} ${CMAKE_BINARY_DIR}/libsndfile_build)

# Upstream only sets HAVE_EXTERNAL_XIPH_LIBS when FLAC, Opus, and Vorbis are all present.
set(SNDFILE_CONFIG "${CMAKE_BINARY_DIR}/libsndfile_build/src/config.h")
file(READ "${SNDFILE_CONFIG}" _cfg)
string(
    REPLACE
    "#define HAVE_EXTERNAL_XIPH_LIBS 0"
    "#define HAVE_EXTERNAL_XIPH_LIBS 1"
    _cfg
    "${_cfg}"
)
file(WRITE "${SNDFILE_CONFIG}" "${_cfg}")

get_target_property(_src sndfile SOURCES)
list(FILTER _src EXCLUDE REGEX ".*/(flac|ogg_opus)\\.c$")
set_property(TARGET sndfile PROPERTY SOURCES "${_src}")
target_sources(sndfile PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sndfile_stubs.c)
target_link_libraries(sndfile PUBLIC ogg vorbis)
