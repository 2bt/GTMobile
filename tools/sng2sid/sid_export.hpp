#pragma once
#include "gtsong.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>


namespace gt {

struct ExportError : std::exception {
    explicit ExportError(std::string m)
        : msg(std::move(m)) {}
    char const* what() const noexcept override { return msg.c_str(); }
    std::string msg;
};

struct ExportOptions {
    enum class Format { Sid, Prg, Bin };
    Format   format              = Format::Sid;
    uint16_t player_addr         = 0x1000;
    uint8_t  zp_base             = 0xfc;
    uint16_t sid_addr            = 0xd400;
    bool     buffered            = false; // delay SID writes until end of each channel
    bool     sound_effects       = false; // include SFX engine (implies buffered)
    bool     volume              = false; // jumptable to set master volume
    bool     author_info         = false; // store author string at player+$20
    bool     zp_ghostregs        = false; // SID writes to ZP instead of $D400 (implies buffered)
    bool     full_buffered       = false; // SID writes to abs ghostregs (implies buffered)
    bool     optimize            = true;  // strip unused effects from the playroutine
    bool     optimize_pulse      = false; // skip idle pulse-table work
    bool     optimize_realtime   = false; // skip idle realtime-effect work
    bool     ntsc                = false; // NTSC CIA/PSID timing (PAL if false)
    int      adparam_override    = -1;    // hardrestart ADSR; -1 = Song::adparam
    int      multiplier_override = -1;    // speed multiplier; -1 = Song::multiplier
};

std::vector<uint8_t> export_song(Song const& song, ExportOptions const& opt = {});

struct AssembleResult {
    std::vector<uint8_t> bytes;
    uint16_t             start = 0;
};

AssembleResult assemble_source(std::string const& source);

std::string load_player_source(bool alt_player);

// Editor rows as note,instr,cmd,data repeating. Returns packed playroutine bytes.
std::vector<uint8_t> pack_pattern(uint8_t const* src,
                                  int            rows,
                                  uint8_t const  instr_map[MAX_INSTR],
                                  uint8_t const  table_map[MAX_TABLES][MAX_TABLELEN + 1],
                                  bool           strip_effects);

} // namespace gt
