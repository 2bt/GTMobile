function(embed_asm var file)
    file(READ "${file}" hex HEX)
    if(hex STREQUAL "")
        message(FATAL_ERROR "failed to read ${file}")
    endif()
    string(REGEX REPLACE "([0-9a-fA-F][0-9a-fA-F])" "0x\\1," hex "${hex}")
    set(${var} "${hex}" PARENT_SCOPE)
endfunction()

function(write_player_embed out_hpp player_asm altplayer_asm)
    embed_asm(PLAYER_ASM_BYTES "${player_asm}")
    embed_asm(ALTPLAYER_ASM_BYTES "${altplayer_asm}")
    file(WRITE "${out_hpp}"
        "static char const PLAYER_ASM[] = {${PLAYER_ASM_BYTES}};\n"
        "static char const ALTPLAYER_ASM[] = {${ALTPLAYER_ASM_BYTES}};\n")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${player_asm}" "${altplayer_asm}")
endfunction()
