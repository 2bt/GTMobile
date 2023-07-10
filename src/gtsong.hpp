#pragma once
#include <cstdint>
#include <istream>


namespace gt {

enum {
    CMD_DONOTHING       = 0,
    CMD_PORTAUP         = 1,
    CMD_PORTADOWN       = 2,
    CMD_TONEPORTA       = 3,
    CMD_VIBRATO         = 4,
    CMD_SETAD           = 5,
    CMD_SETSR           = 6,
    CMD_SETWAVE         = 7,
    CMD_SETWAVEPTR      = 8,
    CMD_SETPULSEPTR     = 9,
    CMD_SETFILTERPTR    = 10,
    CMD_SETFILTERCTRL   = 11,
    CMD_SETFILTERCUTOFF = 12,
    CMD_SETMASTERVOL    = 13,
    CMD_FUNKTEMPO       = 14,
    CMD_SETTEMPO        = 15,

    WTBL = 0,
    PTBL = 1,
    FTBL = 2,
    STBL = 3,

    MAX_FILT         = 64,
    MAX_STR          = 32,
    MAX_INSTR        = 64,
    MAX_CHN          = 3,
    MAX_PATT         = 208,
    MAX_TABLES       = 4,
    MAX_TABLELEN     = 255,
    MAX_INSTRNAMELEN = 16,
    MAX_PATTROWS     = 128,
    MAX_SONGLEN      = 254,
    MAX_SONGS        = 32,
    MAX_NOTES        = 96,

    REPEAT    = 0xd0,
    TRANSDOWN = 0xe0,
    TRANSUP   = 0xf0,
    LOOPSONG  = 0xff,

    ENDPATT   = 0xff,
    FX        = 0x40,
    FXONLY    = 0x50,
    FIRSTNOTE = 0x60,
    LASTNOTE  = 0xbc,
    REST      = 0xbd,
    KEYOFF    = 0xbe,
    KEYON     = 0xbf,

    WAVEDELAY      = 0x1,
    WAVELASTDELAY  = 0xf,
    WAVESILENT     = 0xe0,
    WAVELASTSILENT = 0xef,
    WAVECMD        = 0xf0,
    WAVELASTCMD    = 0xfe,
};


struct Instr {
    uint8_t ad;
    uint8_t sr;
    uint8_t ptr[MAX_TABLES];
    uint8_t vibdelay;
    uint8_t gatetimer;
    uint8_t firstwave;
    char    name[MAX_INSTRNAMELEN];
};


struct Song {
    Instr   instr[MAX_INSTR];
    uint8_t ltable[MAX_TABLES][MAX_TABLELEN];
    uint8_t rtable[MAX_TABLES][MAX_TABLELEN];
    uint8_t songorder[MAX_SONGS][MAX_CHN][MAX_SONGLEN + 2];
    uint8_t pattern[MAX_PATT][MAX_PATTROWS * 4 + 4];

    char    songname[MAX_STR];
    char    authorname[MAX_STR];
    char    copyrightname[MAX_STR];

    int     pattlen[MAX_PATT];
    int     songlen[MAX_SONGS][MAX_CHN];
    int     highestusedpattern;
    int     highestusedinstr;

    void count_pattern_lengths();
    bool load(char const* filename);
    bool load(std::istream& stream);
    bool save(char const* filename);

    void clear();
    void clear_pattern(int p);
    void clear_instr(int num);

    int gettablelen(int num) {
        int c;
        for (c = MAX_TABLELEN - 1; c >= 0; c--) {
            if (ltable[num][c] | rtable[num][c]) break;
        }
        return c + 1;
    }
};


} // namespace gt
