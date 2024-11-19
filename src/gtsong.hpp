#pragma once
#include <cstdint>
#include <istream>
#include <array>
#include <stdexcept>


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

    MAX_STR          = 32,
    MAX_INSTR        = 64,
    MAX_CHN          = 3,
    MAX_PATT         = 208, // actually 0x00 - 0xCF
    MAX_TABLES       = 4,
    MAX_TABLELEN     = 255,
    MAX_INSTRNAMELEN = 16,
    MAX_PATTROWS     = 128,
    MAX_SONGLEN      = 254,
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


struct Instrument {
    uint8_t                            ad        = 0;
    uint8_t                            sr        = 0;
    std::array<uint8_t, 4>             ptr       = {};
    uint8_t                            vibdelay  = 0;
    uint8_t                            gatetimer = 2; // 2 * multiplier
    uint8_t                            firstwave = 0x9;
    std::array<char, MAX_INSTRNAMELEN> name      = {};
};


template<class T, size_t L1, size_t L2>
using Array2 = std::array<std::array<T, L2>, L1>;

struct OrderRow {
    int8_t  trans   = 0;
    uint8_t pattnum = 0;
};

struct PatternRow {
    uint8_t note    = REST;
    uint8_t instr   = 0;
    uint8_t command = 0;
    uint8_t data    = 0;
};

struct Pattern {
    std::array<PatternRow, MAX_PATTROWS> rows = {};
    int                                  len  = 32;
};


struct LoadError : public std::exception {
    LoadError(std::string msg) : msg(std::move(msg)) {}
    const char* what() const noexcept override {
        return msg.c_str();
    }
    std::string msg;
};

struct Song {
    std::array<Instrument, MAX_INSTR>          instruments;
    Array2<uint8_t, MAX_TABLES, MAX_TABLELEN>  ltable;
    Array2<uint8_t, MAX_TABLES, MAX_TABLELEN>  rtable;
    Array2<OrderRow, MAX_CHN, MAX_SONGLEN / 2> song_order;
    std::array<Pattern, MAX_PATT>              patterns;
    int                                        song_len  = 1;
    int                                        song_loop = 0;

    std::array<char, MAX_STR>                  song_name;
    std::array<char, MAX_STR>                  author_name;
    std::array<char, MAX_STR>                  copyright_name;

    void load(char const* filename);
    void load(uint8_t const* data, size_t size);
    void load(std::istream& stream);
    bool save(char const* filename);
    bool save(std::ostream& stream);
    void clear();
};


} // namespace gt
