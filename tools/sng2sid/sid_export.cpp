#include "sid_export.hpp"
#include "player_embed.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

extern "C" {
#include "assembler.h"
}

namespace gt {
namespace {

constexpr int MAX_NOTES = 96;

uint8_t const FREQ_TBL_LO[MAX_NOTES] = {
    0x17, 0x27, 0x39, 0x4b, 0x5f, 0x74, 0x8a, 0xa1, 0xba, 0xd4, 0xf0, 0x0e, //
    0x2d, 0x4e, 0x71, 0x96, 0xbe, 0xe8, 0x14, 0x43, 0x74, 0xa9, 0xe1, 0x1c, //
    0x5a, 0x9c, 0xe2, 0x2d, 0x7c, 0xcf, 0x28, 0x85, 0xe8, 0x52, 0xc1, 0x37, //
    0xb4, 0x39, 0xc5, 0x5a, 0xf7, 0x9e, 0x4f, 0x0a, 0xd1, 0xa3, 0x82, 0x6e, //
    0x68, 0x71, 0x8a, 0xb3, 0xee, 0x3c, 0x9e, 0x15, 0xa2, 0x46, 0x04, 0xdc, //
    0xd0, 0xe2, 0x14, 0x67, 0xdd, 0x79, 0x3c, 0x29, 0x44, 0x8d, 0x08, 0xb8, //
    0xa1, 0xc5, 0x28, 0xcd, 0xba, 0xf1, 0x78, 0x53, 0x87, 0x1a, 0x10, 0x71, //
    0x42, 0x89, 0x4f, 0x9b, 0x74, 0xe2, 0xf0, 0xa6, 0x0e, 0x33, 0x20, 0xff, //
};
uint8_t const FREQ_TBL_HI[MAX_NOTES] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, //
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, //
    0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x07, 0x07, 0x08, //
    0x08, 0x09, 0x09, 0x0a, 0x0a, 0x0b, 0x0c, 0x0d, 0x0d, 0x0e, 0x0f, 0x10, //
    0x11, 0x12, 0x13, 0x14, 0x15, 0x17, 0x18, 0x1a, 0x1b, 0x1d, 0x1f, 0x20, //
    0x22, 0x24, 0x27, 0x29, 0x2b, 0x2e, 0x31, 0x34, 0x37, 0x3a, 0x3e, 0x41, //
    0x45, 0x49, 0x4e, 0x52, 0x57, 0x5c, 0x62, 0x68, 0x6e, 0x75, 0x7c, 0x83, //
    0x8b, 0x93, 0x9c, 0xa5, 0xaf, 0xb9, 0xc4, 0xd0, 0xdd, 0xea, 0xf8, 0xff, //
};

char const* const TABLE_LEFT_NAME[] = {
    "mt_wavetbl",
    "mt_pulsetimetbl",
    "mt_filttimetbl",
    "mt_speedlefttbl",
};
char const* const TABLE_RIGHT_NAME[] = {
    "mt_notetbl",
    "mt_pulsespdtbl",
    "mt_filtspdtbl",
    "mt_speedrighttbl",
};

uint8_t swap_nybbles(uint8_t n) { return uint8_t((n & 0xf) << 4) | (n >> 4); }

struct AssembleResult {
    std::vector<uint8_t> bytes;
    uint16_t             start = 0;
};

std::string load_player_source(bool alt_player) {
    if (alt_player) return { ALTPLAYER_ASM, sizeof(ALTPLAYER_ASM) };
    return { PLAYER_ASM, sizeof(PLAYER_ASM) };
}

class AsmSrc {
public:
    void label(char const* n) {
        s += n;
        s += ":\n";
    }
    void byte(uint8_t b) {
        char buf[32];
        snprintf(buf, sizeof buf, "                !byte $%02x\n", b);
        s += buf;
    }
    void bytes(uint8_t const* p, int n) {
        constexpr int ROW = 16;
        for (int i = 0; i < n;) {
            int row = std::min(ROW, n - i);
            s += "                !byte ";
            for (int j = 0; j < row; ++j) {
                char buf[16];
                snprintf(buf, sizeof buf, "%s$%02x", j ? "," : "", p[i + j]);
                s += buf;
            }
            s += "\n";
            i += row;
        }
    }
    void addr_lo(std::string const& n) {
        s += "                !byte <";
        s += n;
        s += "\n";
    }
    void addr_hi(std::string const& n) {
        s += "                !byte >";
        s += n;
        s += "\n";
    }
    void def(char const* n, int v) {
        char buf[64];
        snprintf(buf, sizeof buf, "%s = %d\n", n, v);
        s += buf;
    }
    void def(char const* n, bool v) { def(n, int(v)); }
    void player(std::string const& src) {
        s += src;
        s += '\n';
    }
    AssembleResult assemble() const {
        Assembler* as = assembler_create();
        if (!as) throw ExportError("assembler_create failed");
        int rc = assembler_assemble_string(as, s.c_str(), "sng2sid.asm");
        if (rc != 0 || assembler_has_errors(as)) {
            int n = assembler_error_count(as);
            assembler_free(as);
            throw ExportError("assemble failed with " + std::to_string(n) + " error(s)");
        }
        uint16_t       start = 0;
        int            size  = 0;
        uint8_t const* bytes = assembler_get_output(as, &start, &size);
        AssembleResult r;
        r.start = start;
        r.bytes.assign(bytes, bytes + size);
        assembler_free(as);
        return r;
    }
private:
    std::string s;
};

struct PackFlags {
    bool no_effects          = true;
    bool no_gate             = true;
    bool no_filter           = true;
    bool no_filter_mod       = true;
    bool no_pulse            = true;
    bool no_pulse_mod        = true;
    bool no_wave_delay       = true;
    bool no_wave_cmd         = true;
    bool no_repeat           = true;
    bool no_trans            = true;
    bool no_portamento       = true;
    bool no_tone_porta       = true;
    bool no_vib              = true;
    bool no_ins_vib          = true;
    bool no_set_ad           = true;
    bool no_set_sr           = true;
    bool no_set_wave         = true;
    bool no_set_wave_ptr     = true;
    bool no_set_pulse_ptr    = true;
    bool no_set_filt_ptr     = true;
    bool no_set_filt_cutoff  = true;
    bool no_set_filt_ctrl    = true;
    bool no_set_master_vol   = true;
    bool no_funk_tempo       = true;
    bool no_global_tempo     = true;
    bool no_channel_tempo    = true;
    bool no_first_wave_cmd   = true;
    bool no_calculated_speed = true;
    bool no_normal_speed     = true;
    bool no_zero_speed       = true;
    bool author_info         = false;
};

// Pack editor rows into playroutine bytes and record which effects were used.
std::vector<uint8_t> pack_pattern_raw(Pattern         patt,
                                      uint8_t const*  instr_map,
                                      uint8_t const   table_map[MAX_TABLES][MAX_TABLELEN + 1],
                                      PackFlags&      f) {
    uint8_t last_instr = 0;
    for (int i = 0; i < patt.len; ++i) {
        PatternRow& r = patt.rows[i];
        if (r.instr && r.instr == last_instr) r.instr = 0;
        else if (r.instr)
            last_instr = r.instr;

        auto remap = [&](int table) { r.data = table_map[table][r.data]; };
        switch (r.command) {
        case CMD_PORTAUP:
        case CMD_PORTADOWN:
            f.no_portamento = false;
            remap(STBL);
            break;
        case CMD_TONEPORTA:
            f.no_tone_porta = false;
            remap(STBL);
            break;
        case CMD_VIBRATO:
            f.no_vib = false;
            remap(STBL);
            break;
        case CMD_SETAD: f.no_set_ad = false; break;
        case CMD_SETSR: f.no_set_sr = false; break;
        case CMD_SETWAVE: f.no_set_wave = false; break;
        case CMD_SETWAVEPTR:
            f.no_set_wave_ptr = false;
            remap(WTBL);
            break;
        case CMD_SETPULSEPTR:
            f.no_set_pulse_ptr = false;
            f.no_pulse         = false;
            remap(PTBL);
            break;
        case CMD_SETFILTERPTR:
            f.no_set_filt_ptr = false;
            f.no_filter       = false;
            remap(FTBL);
            break;
        case CMD_SETFILTERCTRL:
            f.no_set_filt_ctrl = false;
            f.no_filter        = false;
            break;
        case CMD_SETFILTERCUTOFF:
            f.no_set_filt_cutoff = false;
            f.no_filter          = false;
            break;
        case CMD_SETMASTERVOL:
            f.no_set_master_vol = false;
            if (!f.author_info && r.data > 0x0f) {
                r.command = 0;
                r.data    = 0;
            }
            break;
        case CMD_FUNKTEMPO:
            f.no_funk_tempo = false;
            remap(STBL);
            break;
        case CMD_SETTEMPO:
            if (r.data >= 0x80) f.no_channel_tempo = false;
            else
                f.no_global_tempo = false;
            if ((r.data & 0x7f) >= 3) r.data--;
            break;
        }
    }

    int cmd  = f.no_effects ? 0 : -1;
    int data = f.no_effects ? 0 : -1;
    std::vector<uint8_t> mid;
    auto emit_fx = [&](uint8_t prefix, PatternRow const& r) {
        if (r.command == cmd && r.data == data) return false;
        cmd  = r.command;
        data = r.data;
        mid.push_back(uint8_t(prefix + cmd));
        if (cmd) mid.push_back(uint8_t(data));
        return true;
    };
    for (int i = 0; i < patt.len; ++i) {
        PatternRow const& r = patt.rows[i];
        if (r.instr) mid.push_back(instr_map[r.instr]);
        if (r.note == REST) {
            if (!emit_fx(FXONLY, r)) mid.push_back(REST);
        }
        else {
            emit_fx(FX, r);
            mid.push_back(r.note);
        }
    }

    std::vector<uint8_t> dest;
    for (size_t i = 0; i < mid.size();) {
        bool pack = i != 0;
        if (mid[i] < FX) {
            dest.push_back(mid[i++]);
            pack = false;
        }
        if (i < mid.size() && mid[i] >= FXONLY && mid[i] < FIRSTNOTE) {
            int fx = mid[i] - FXONLY;
            dest.push_back(mid[i++]);
            if (fx) dest.push_back(mid[i++]);
            continue;
        }
        if (i < mid.size() && mid[i] < FXONLY) {
            int fx = mid[i] - FX;
            dest.push_back(mid[i++]);
            if (fx) dest.push_back(mid[i++]);
            pack = false;
        }
        if (i >= mid.size()) break;
        if (!pack || mid[i] != REST) {
            dest.push_back(mid[i++]);
            continue;
        }
        size_t n = 0;
        while (i + n < mid.size() && mid[i + n] == REST && n < 64) n++;
        if (n > 1) {
            dest.push_back(uint8_t(-int(n)));
            i += n;
        }
        else {
            dest.push_back(mid[i++]);
        }
    }
    if (dest.size() > 256) throw ExportError("pattern too complex (over 256 bytes packed)");
    if (dest.size() < 256) dest.push_back(0x00);
    return dest;
}

class Packer {
public:
    Packer(Song const& song, ExportOptions const& opt) : m_song(song), m_opt(opt) {}

    // Pack the song, assemble the player, wrap SID/PRG/BIN.
    std::vector<uint8_t> build() {
        scan();

        // Overrides, buffering, and channel count (SFX/ghostregs need all 3).
        int adparam    = m_opt.adparam_override    >= 0 ? m_opt.adparam_override    : m_song.adparam;
        int multiplier = m_opt.multiplier_override >= 0 ? m_opt.multiplier_override : m_song.multiplier;
        if (multiplier > 16) multiplier = 16;

        bool sfx      = m_opt.sound_effects;
        bool ghost_zp = m_opt.zp_ghostregs;
        bool full_buf = m_opt.full_buffered;
        bool buffered = m_opt.buffered || sfx || ghost_zp || full_buf;
        if (sfx || full_buf || ghost_zp) m_channels = 3;

        if (!m_opt.optimize) {
            // Keep every playroutine feature, even if the song does not use it.
            m_fixed_params = 0;
            if (!m_num_legato) m_num_legato++;
            m_simple_pulse = 0;
            m_first_note   = 0;
            m_last_note    = MAX_NOTES - 1;
            m_flags.no_effects          = false;
            m_flags.no_gate             = false;
            m_flags.no_filter           = false;
            m_flags.no_filter_mod       = false;
            m_flags.no_pulse            = false;
            m_flags.no_pulse_mod        = false;
            m_flags.no_wave_delay       = false;
            m_flags.no_wave_cmd         = false;
            m_flags.no_repeat           = false;
            m_flags.no_trans            = false;
            m_flags.no_portamento       = false;
            m_flags.no_tone_porta       = false;
            m_flags.no_vib              = false;
            m_flags.no_ins_vib          = false;
            m_flags.no_set_ad           = false;
            m_flags.no_set_sr           = false;
            m_flags.no_set_wave         = false;
            m_flags.no_set_wave_ptr     = false;
            m_flags.no_set_pulse_ptr    = false;
            m_flags.no_set_filt_ptr     = false;
            m_flags.no_set_filt_cutoff  = false;
            m_flags.no_set_filt_ctrl    = false;
            m_flags.no_set_master_vol   = false;
            m_flags.no_funk_tempo       = false;
            m_flags.no_global_tempo     = false;
            m_flags.no_channel_tempo    = false;
            m_flags.no_first_wave_cmd   = false;
            m_flags.no_calculated_speed = false;
            m_flags.no_normal_speed     = false;
            m_flags.no_zero_speed       = false;
        }

        // Orderlists: transpose bytes, remapped pattern indices, loop point.
        std::vector<uint8_t> song_work;
        int                  song_offset[MAX_CHN]{};
        int                  song_size[MAX_CHN]{};
        for (int d = 0; d < MAX_CHN; d++) {
            song_offset[d] = int(song_work.size());
            int trans      = 0;
            int loop       = m_song.song_loop;
            for (int r = 0; r < m_song.song_len; r++) {
                OrderRow const& row = m_song.song_order[d][r];
                if (row.trans != trans) {
                    trans = row.trans;
                    song_work.push_back(uint8_t(trans + TRANSUP));
                    if (r < m_song.song_loop) ++loop;
                }
                song_work.push_back(m_patt_map[row.pattnum]);
            }
            song_work.push_back(LOOPSONG);
            song_work.push_back(uint8_t(loop));
            song_size[d] = int(song_work.size()) - song_offset[d];
            if (loop >= song_size[d] - 2) throw ExportError("illegal song restart position");
        }

        // Used patterns, packed for the playroutine.
        std::vector<uint8_t> patt_work;
        std::vector<int>     patt_offset;
        std::vector<int>     patt_size;
        for (int c = 0; c < MAX_PATT; c++) {
            if (!m_patt_used[c]) continue;
            auto packed = pack_pattern_raw(m_song.patterns[c], m_instr_map, m_table_map, m_flags);
            patt_offset.push_back(int(patt_work.size()));
            patt_size.push_back(int(packed.size()));
            patt_work.insert(patt_work.end(), packed.begin(), packed.end());
        }

        // Instruments as 9 parallel arrays: AD, SR, table ptrs, vib, gatetimer, firstwave.
        std::vector<uint8_t> instr_work(m_instruments * 9, 0);
        for (size_t c = 1; c < MAX_INSTR; c++) {
            if (!m_instr_used[c]) continue;
            Instrument const& ins = m_song.instruments[c];
            size_t d = m_instr_map[c] - 1;
            instr_work[d + m_instruments * 0] = ins.ad;
            instr_work[d + m_instruments * 1] = ins.sr;
            instr_work[d + m_instruments * 2] = m_table_map[WTBL][ins.ptr[WTBL]];
            instr_work[d + m_instruments * 3] = m_table_map[PTBL][ins.ptr[PTBL]];
            instr_work[d + m_instruments * 4] = m_table_map[FTBL][ins.ptr[FTBL]];
            if (ins.vibdelay) {
                instr_work[d + m_instruments * 5] = m_table_map[STBL][ins.ptr[STBL]];
                instr_work[d + m_instruments * 6] = uint8_t(ins.vibdelay - 1);
            }
            instr_work[d + m_instruments * 7] = ins.gatetimer & 0x3f;
            instr_work[d + m_instruments * 8] = ins.firstwave;
            if (ins.ptr[STBL] && ins.vibdelay) {
                m_flags.no_vib     = false;
                m_flags.no_ins_vib = false;
            }
            if (ins.ptr[PTBL]) m_flags.no_pulse = false;
            if (ins.ptr[FTBL]) m_flags.no_filter = false;
            if (ins.gatetimer != m_song.instruments[1].gatetimer || ins.firstwave != m_song.instruments[1].firstwave)
                m_fixed_params = 0;
            if (!ins.firstwave || ins.firstwave >= 0xfe) m_fixed_params = 0;
        }

        if (multiplier > 1) {
            // Dummy extra instruments so the CIA-multiplied player can still index them.
            m_fixed_params = 0;
            m_num_legato++;
            m_num_no_hr++;
        }

        auto const& ltable = m_song.ltable;
        auto const& rtable = m_song.rtable;
        // Tables can still enable player features and widen the frequency range.
        for (int c = 0; c < MAX_TABLELEN; c++) {
            if (!m_table_used[WTBL][c + 1]) continue;
            if (ltable[WTBL][c] >= WAVEDELAY && ltable[WTBL][c] <= WAVELASTDELAY) m_flags.no_wave_delay = false;
            if (ltable[WTBL][c] >= WAVECMD && ltable[WTBL][c] <= WAVELASTCMD) {
                m_flags.no_wave_cmd = false;
                m_flags.no_effects  = false;
                switch (ltable[WTBL][c] - WAVECMD) {
                case CMD_PORTAUP: // fall through
                case CMD_PORTADOWN:       m_flags.no_portamento      = false; break;
                case CMD_TONEPORTA:       m_flags.no_tone_porta      = false; break;
                case CMD_VIBRATO:         m_flags.no_vib             = false; break;
                case CMD_SETAD:           m_flags.no_set_ad          = false; break;
                case CMD_SETSR:           m_flags.no_set_sr          = false; break;
                case CMD_SETWAVE:         m_flags.no_set_wave        = false; break;
                case CMD_SETPULSEPTR:     m_flags.no_set_pulse_ptr   = false; break;
                case CMD_SETFILTERPTR:    m_flags.no_set_filt_ptr    = false; break;
                case CMD_SETFILTERCUTOFF: m_flags.no_set_filt_cutoff = false; break;
                case CMD_SETFILTERCTRL:   m_flags.no_set_filt_ctrl   = false; break;
                case CMD_SETMASTERVOL:    m_flags.no_set_master_vol  = false; break;
                }
            }
            if (ltable[WTBL][c] < WAVECMD) {
                if (rtable[WTBL][c] <= 0x80) {
                    int new_last = rtable[WTBL][c] + m_pattern_last_note;
                    if (new_last > MAX_NOTES - 1) new_last = MAX_NOTES - 1;
                    if (rtable[WTBL][c] >= 0x20) m_first_note = 0;
                    if (new_last > m_last_note) m_last_note = new_last;
                }
                else {
                    int nn = rtable[WTBL][c] & 0x7f;
                    if (nn > MAX_NOTES - 1) nn = MAX_NOTES - 1;
                    if (nn < m_first_note) m_first_note = nn;
                    if (nn > m_last_note) m_last_note = nn;
                }
            }
        }
        for (int c = 0; c < MAX_TABLELEN; c++) {
            if (!m_table_used[PTBL][c + 1]) continue;
            if (ltable[PTBL][c] >= 0x80 && ltable[PTBL][c] != 0xff) {
                if (rtable[PTBL][c] & 0xf) m_simple_pulse = 0;
            }
            if (ltable[PTBL][c] < 0x80) {
                m_flags.no_pulse_mod = false;
                if (rtable[PTBL][c] & 0xf) m_simple_pulse = 0;
            }
        }
        for (int c = 0; c < MAX_TABLELEN; c++) {
            if (m_table_used[FTBL][c + 1] && ltable[FTBL][c] < 0x80) m_flags.no_filter_mod = false;
        }

        // Clip the frequency table to the notes actually needed (full range for SFX).
        if (m_last_note < m_first_note) m_last_note = m_first_note;
        if (m_first_note < 0) m_first_note = 0;
        if (!m_flags.no_calculated_speed) m_last_note++;
        if (m_last_note > MAX_NOTES - 1) m_last_note = MAX_NOTES - 1;
        if (sfx) {
            m_first_note = 0;
            m_last_note  = MAX_NOTES - 1;
        }

        // Player assembler defines: load addresses, feature flags, instrument counts.
        AsmSrc data;
        data.def("base", m_opt.player_addr);
        data.def("zpbase", m_opt.zp_base);
        data.def("SIDBASE", m_opt.sid_addr);
        data.def("SOUNDSUPPORT", sfx);
        data.def("VOLSUPPORT", m_opt.volume);
        data.def("BUFFEREDWRITES", buffered);
        data.def("GHOSTREGS", ghost_zp || full_buf);
        data.def("ZPGHOSTREGS", ghost_zp);
        data.def("FIXEDPARAMS", m_fixed_params);
        data.def("SIMPLEPULSE", m_simple_pulse);
        data.def("PULSEOPTIMIZATION", m_opt.optimize_pulse);
        data.def("REALTIMEOPTIMIZATION", m_opt.optimize_realtime);
        data.def("NOAUTHORINFO", !m_opt.author_info);
        data.def("NOEFFECTS", m_flags.no_effects);
        data.def("NOGATE", m_flags.no_gate);
        data.def("NOFILTER", m_flags.no_filter);
        data.def("NOFILTERMOD", m_flags.no_filter_mod);
        data.def("NOPULSE", m_flags.no_pulse);
        data.def("NOPULSEMOD", m_flags.no_pulse_mod);
        data.def("NOWAVEDELAY", m_flags.no_wave_delay);
        data.def("NOWAVECMD", m_flags.no_wave_cmd);
        data.def("NOREPEAT", m_flags.no_repeat);
        data.def("NOTRANS", m_flags.no_trans);
        data.def("NOPORTAMENTO", m_flags.no_portamento);
        data.def("NOTONEPORTA", m_flags.no_tone_porta);
        data.def("NOVIB", m_flags.no_vib);
        data.def("NOINSTRVIB", m_flags.no_ins_vib);
        data.def("NOSETAD", m_flags.no_set_ad);
        data.def("NOSETSR", m_flags.no_set_sr);
        data.def("NOSETWAVE", m_flags.no_set_wave);
        data.def("NOSETWAVEPTR", m_flags.no_set_wave_ptr);
        data.def("NOSETPULSEPTR", m_flags.no_set_pulse_ptr);
        data.def("NOSETFILTPTR", m_flags.no_set_filt_ptr);
        data.def("NOSETFILTCTRL", m_flags.no_set_filt_ctrl);
        data.def("NOSETFILTCUTOFF", m_flags.no_set_filt_cutoff);
        data.def("NOSETMASTERVOL", m_flags.no_set_master_vol);
        data.def("NOFUNKTEMPO", m_flags.no_funk_tempo);
        data.def("NOGLOBALTEMPO", m_flags.no_global_tempo);
        data.def("NOCHANNELTEMPO", m_flags.no_channel_tempo);
        data.def("NOFIRSTWAVECMD", m_flags.no_first_wave_cmd);
        data.def("NOCALCULATEDSPEED", m_flags.no_calculated_speed);
        data.def("NONORMALSPEED", m_flags.no_normal_speed);
        data.def("NOZEROSPEED", m_flags.no_zero_speed);
        data.def("NUMCHANNELS", m_channels);
        data.def("NUMSONGS", 1);
        data.def("FIRSTNOTE", m_first_note);
        data.def("FIRSTNOHRINSTR", m_num_normal + 1);
        data.def("FIRSTLEGATOINSTR", m_num_normal + m_num_no_hr + 1);
        data.def("NUMHRINSTR", m_num_normal);
        data.def("NUMNOHRINSTR", m_num_no_hr);
        data.def("NUMLEGATOINSTR", m_num_legato);
        data.def("ADPARAM", (adparam >> 8) & 0xff);
        data.def("SRPARAM", adparam & 0xff);
        if (m_song.instruments[MAX_INSTR - 1].ad >= 2 && !m_song.instruments[MAX_INSTR - 1].ptr[WTBL]) {
            data.def("DEFAULTTEMPO", m_song.instruments[MAX_INSTR - 1].ad - 1);
        }
        else {
            data.def("DEFAULTTEMPO", multiplier ? (multiplier * 6 - 1) : 5);
        }
        if (m_fixed_params) {
            data.def("FIRSTWAVEPARAM", m_song.instruments[1].firstwave);
            data.def("GATETIMERPARAM", m_song.instruments[1].gatetimer & 0x3f);
        }

        // Player source and frequency table for the used note range.
        std::string player = load_player_source(adparam >= 0xf000);
        data.player(player);
        data.label("mt_freqtbllo");
        data.bytes(&FREQ_TBL_LO[m_first_note], m_last_note - m_first_note + 1);
        data.label("mt_freqtblhi");
        data.bytes(&FREQ_TBL_HI[m_first_note], m_last_note - m_first_note + 1);

        // Orderlist and pattern address tables.
        data.label("mt_songtbllo");
        for (int c = 0; c < 3; c++) data.addr_lo("mt_song" + std::to_string(c));
        data.label("mt_songtblhi");
        for (int c = 0; c < 3; c++) data.addr_hi("mt_song" + std::to_string(c));

        data.label("mt_patttbllo");
        for (int c = 0; c < m_patterns; c++) data.addr_lo("mt_patt" + std::to_string(c));
        data.label("mt_patttblhi");
        for (int c = 0; c < m_patterns; c++) data.addr_hi("mt_patt" + std::to_string(c));

        // Instrument tables (skip columns the playroutine was built without).
        data.label("mt_insad");
        data.bytes(&instr_work[0], m_instruments);
        data.label("mt_inssr");
        data.bytes(&instr_work[size_t(m_instruments)], m_instruments);
        data.label("mt_inswaveptr");
        data.bytes(&instr_work[size_t(m_instruments) * 2], m_instruments);
        if (!m_flags.no_pulse) {
            data.label("mt_inspulseptr");
            data.bytes(&instr_work[size_t(m_instruments) * 3], m_instruments);
        }
        if (!m_flags.no_filter) {
            data.label("mt_insfiltptr");
            data.bytes(&instr_work[size_t(m_instruments) * 4], m_instruments);
        }
        if (!m_flags.no_ins_vib) {
            data.label("mt_insvibparam");
            data.bytes(&instr_work[size_t(m_instruments) * 5], m_instruments);
            data.label("mt_insvibdelay");
            data.bytes(&instr_work[size_t(m_instruments) * 6], m_instruments);
        }
        if (!m_fixed_params) {
            data.label("mt_insgatetimer");
            data.bytes(&instr_work[size_t(m_instruments) * 7], m_instruments);
            data.label("mt_insfirstwave");
            data.bytes(&instr_work[size_t(m_instruments) * 8], m_instruments);
        }

        // Wave/pulse/filter/speed tables: used rows only, jumps remapped.
        bool speed_extra = !m_flags.no_vib || !m_flags.no_funk_tempo || !m_flags.no_portamento || !m_flags.no_tone_porta;
        for (int c = 0; c < MAX_TABLES; c++) {
            if (c == PTBL && m_flags.no_pulse) continue;
            if (c == FTBL && m_flags.no_filter) continue;
            if (c == STBL && speed_extra) data.byte(0);

            data.label(TABLE_LEFT_NAME[c]);
            for (int d = 0; d < MAX_TABLELEN; d++) {
                if (!m_table_used[c][d + 1]) continue;
                switch (c) {
                case WTBL: {
                    uint8_t wave = ltable[c][d];
                    if (ltable[c][d] >= WAVESILENT && ltable[c][d] <= WAVELASTSILENT) wave &= 0xf;
                    if (ltable[c][d] > WAVELASTDELAY && ltable[c][d] <= WAVELASTSILENT && !m_flags.no_wave_delay)
                        wave += 0x10;
                    data.byte(wave);
                    break;
                }
                case PTBL:
                    if (m_simple_pulse && ltable[c][d] != 0xff && ltable[c][d] > 0x80) data.byte(0x80);
                    else
                        data.byte(ltable[c][d]);
                    break;
                case FTBL:
                    if (ltable[c][d] != 0xff && ltable[c][d] > 0x80)
                        data.byte(uint8_t(((ltable[c][d] & 0x70) >> 1) | 0x80));
                    else
                        data.byte(ltable[c][d]);
                    break;
                default: data.byte(ltable[c][d]); break;
                }
            }
            if (c == STBL && speed_extra) data.byte(0);

            data.label(TABLE_RIGHT_NAME[c]);
            for (int d = 0; d < MAX_TABLELEN; d++) {
                if (!m_table_used[c][d + 1]) continue;
                if (ltable[c][d] != 0xff || c == STBL) {
                    switch (c) {
                    case WTBL:
                        if (ltable[c][d] >= WAVECMD && ltable[c][d] <= WAVELASTCMD) {
                            switch (ltable[c][d] - WAVECMD) {
                            case CMD_PORTAUP:
                            case CMD_PORTADOWN:
                            case CMD_TONEPORTA:
                            case CMD_VIBRATO: data.byte(m_table_map[STBL][rtable[c][d]]); break;
                            case CMD_SETPULSEPTR: data.byte(m_table_map[PTBL][rtable[c][d]]); break;
                            case CMD_SETFILTERPTR: data.byte(m_table_map[FTBL][rtable[c][d]]); break;
                            default: data.byte(rtable[c][d]); break;
                            }
                        }
                        else {
                            data.byte(rtable[c][d] ^ 0x80);
                        }
                        break;
                    case PTBL:
                        if (m_simple_pulse) {
                            if (ltable[c][d] >= 0x80)
                                data.byte(uint8_t((ltable[c][d] & 0x0f) | (rtable[c][d] & 0xf0)));
                            else {
                                int pulse_speed = rtable[c][d] >> 4;
                                if (rtable[c][d] & 0x80) {
                                    pulse_speed |= 0xf0;
                                    pulse_speed--;
                                }
                                data.byte(swap_nybbles(uint8_t(pulse_speed)));
                            }
                        }
                        else {
                            data.byte(rtable[c][d]);
                        }
                        break;
                    default: data.byte(rtable[c][d]); break;
                    }
                }
                else {
                    data.byte(m_table_map[c][rtable[c][d]]);
                }
            }
        }

        // Orderlist and pattern bodies.
        for (int d = 0; d < MAX_CHN; d++) {
            std::string n = "mt_song" + std::to_string(d);
            data.label(n.c_str());
            data.bytes(&song_work[size_t(song_offset[d])], song_size[d]);
        }
        for (int c = 0; c < m_patterns; c++) {
            std::string n = "mt_patt" + std::to_string(c);
            data.label(n.c_str());
            data.bytes(&patt_work[size_t(patt_offset[c])], patt_size[c]);
        }

        AssembleResult assembled = data.assemble();
        if (m_opt.author_info) {
            // Author string lives at player+$20 when that feature is on.
            size_t off = 32;
            if (assembled.bytes.size() > off + 32) {
                for (size_t c = 0; c < 32; c++) {
                    char ch = m_song.author_name[c];
                    assembled.bytes[off + c] = ch ? ch : 0x20;
                }
            }
        }
        return wrap_output(assembled.bytes, multiplier);
    }

private:

    // Mark used channels/patterns/instruments/tables, and which player features to keep.
    void scan() {
        auto const& ltable = m_song.ltable;
        auto const& rtable = m_song.rtable;
        m_flags.author_info      = m_opt.author_info;

        if (m_song.song_len <= 0) throw ExportError("no songs, no data to save");

        for (int d = 0; d < MAX_CHN; d++) {
            int trans = 0;
            for (int r = 0; r < m_song.song_len; r++) {
                OrderRow const& row = m_song.song_order[d][r];
                if (row.trans != trans) {
                    m_flags.no_trans = false;
                    trans      = row.trans;
                    if (trans < 0) {
                        int nd = -trans;
                        if (nd > m_trans_down_range) m_trans_down_range = nd;
                    }
                    else if (trans > m_trans_up_range) {
                        m_trans_up_range = trans;
                    }
                }
                uint8_t num = row.pattnum;
                if (num >= MAX_PATT) throw ExportError("invalid pattern number in orderlist");
                m_patt_used[num]      = 1;
                Pattern const& patt = m_song.patterns[num];
                for (int k = 0; k < patt.len; k++) {
                    PatternRow const& pr = patt.rows[k];
                    if (pr.note != REST || pr.instr || pr.command) m_chn_used[d] = 1;
                }
            }
        }

        if (!m_chn_used[2]) m_channels = 2;
        if (!m_chn_used[1] && !m_chn_used[2]) m_channels = 1;

        m_instr_used[1] = 1;
        for (int c = 0; c < MAX_PATT; c++) {
            if (!m_patt_used[c]) continue;
            m_patt_map[c] = uint8_t(m_patterns++);
            Pattern const& patt = m_song.patterns[c];
            for (int d = 0; d < patt.len; d++) {
                PatternRow const& pr = patt.rows[d];
                uint8_t note = pr.note;
                uint8_t ins  = pr.instr;
                uint8_t cmd  = pr.command;
                uint8_t data = pr.data;
                if (note == KEYOFF || note == KEYON) m_flags.no_gate = false;
                if (ins) m_instr_used[ins] = 1;
                if (cmd) m_flags.no_effects = false;
                if (cmd >= CMD_SETWAVEPTR && cmd <= CMD_SETFILTERPTR)
                    exec_table(cmd - CMD_SETWAVEPTR, data);
                if (cmd >= CMD_PORTAUP && cmd <= CMD_VIBRATO) {
                    exec_table(STBL, data);
                    calc_speed_test(data);
                }
                if (cmd == CMD_FUNKTEMPO) {
                    exec_table(STBL, data);
                    m_flags.no_funk_tempo   = false;
                    m_flags.no_global_tempo = false;
                }
                if (cmd == CMD_SETTEMPO && (data & 0x7f) < 3) m_flags.no_funk_tempo = false;
                if (note >= FIRSTNOTE && note <= LASTNOTE) {
                    int new_first = note - FIRSTNOTE - m_trans_down_range;
                    int new_last  = note - FIRSTNOTE + m_trans_up_range;
                    if (new_first < 0) new_first = 0;
                    if (new_last > MAX_NOTES - 1) new_last = MAX_NOTES - 1;
                    if (new_first < m_first_note) m_first_note = new_first;
                    if (new_last > m_last_note) {
                        m_pattern_last_note = new_last;
                        m_last_note         = new_last;
                    }
                    if (new_first > m_last_note) {
                        m_pattern_last_note = new_first;
                        m_last_note         = new_first;
                    }
                }
            }
        }

        for (int c = 0; c < MAX_INSTR; c++) {
            if (!m_instr_used[c]) continue;
            if (m_song.instruments[c].gatetimer & 0x40) m_num_legato++;
            else if (m_song.instruments[c].gatetimer & 0x80)
                m_num_no_hr++;
            else
                m_num_normal++;
            uint8_t fw = m_song.instruments[c].firstwave;
            if (!fw || fw >= 0xfe) m_flags.no_first_wave_cmd = false;
        }
        int free_normal = 1;
        int free_no_hr  = free_normal + m_num_normal;
        int free_legato = free_no_hr + m_num_no_hr;
        for (int c = 0; c < MAX_INSTR; c++) {
            if (!m_instr_used[c]) continue;
            if (m_song.instruments[c].gatetimer & 0x40) m_instr_map[c] = uint8_t(free_legato++);
            else if (m_song.instruments[c].gatetimer & 0x80)
                m_instr_map[c] = uint8_t(free_no_hr++);
            else
                m_instr_map[c] = uint8_t(free_normal++);
            m_instruments++;
            for (int d = 0; d < MAX_TABLES; d++) {
                uint8_t ptr = m_song.instruments[c].ptr[d];
                if (d == STBL && m_song.instruments[c].vibdelay == 0 &&
                    (ptr == 0 || (ltable[STBL][ptr - 1] == 0 && rtable[STBL][ptr - 1] == 0)))
                    continue;
                exec_table(d, ptr);
                if (d == STBL) calc_speed_test(ptr);
            }
        }

        for (int c = 0; c < MAX_TABLELEN; c++) {
            if (!m_table_used[WTBL][c + 1]) continue;
            if (ltable[WTBL][c] < WAVECMD || ltable[WTBL][c] > WAVELASTCMD) continue;
            int d = -1;
            switch (ltable[WTBL][c] - WAVECMD) {
            case CMD_PORTAUP:
            case CMD_PORTADOWN:
            case CMD_TONEPORTA:
            case CMD_VIBRATO:
                d = STBL;
                calc_speed_test(rtable[WTBL][c]);
                break;
            case CMD_SETPULSEPTR:
                d          = PTBL;
                m_flags.no_pulse = false;
                break;
            case CMD_SETFILTERPTR:
                d           = FTBL;
                m_flags.no_filter = false;
                break;
            case CMD_DONOTHING:
            case CMD_SETWAVEPTR:
            case CMD_FUNKTEMPO: throw ExportError("illegal wavetable command");
            }
            if (d != -1) exec_table(d, rtable[WTBL][c]);
        }

        for (int c = 0; c < MAX_TABLES; c++) {
            int e = 1;
            for (int d = 0; d < MAX_TABLELEN; d++) {
                if (m_table_used[c][d + 1]) m_table_map[c][d + 1] = uint8_t(e++);
            }
        }
        for (int c = 0; c < MAX_TABLES; c++) find_table_duplicates(c);
    }

    // Rows from pos through the terminating $ff jump (speed table: always 1).
    int table_part_len(int table, int pos) const {
        if (pos < 0) return 0;
        if (table == STBL) return 1;
        int c;
        for (c = pos; c < MAX_TABLELEN; c++) {
            if (m_song.ltable[table][c] == 0xff) {
                c++;
                break;
            }
        }
        return c - pos;
    }

    // Mark table rows reachable from ptr.
    void exec_table(int table, int ptr) {
        if (table != STBL && ptr && ptr <= MAX_TABLELEN) {
            if (m_song.ltable[table][ptr - 1] == 0xff) throw ExportError("table pointer points to a jump");
        }
        for (;;) {
            if (!ptr) break;
            if (table != STBL && ptr > MAX_TABLELEN) throw ExportError("table execution overflows");
            if (m_table_used[table][ptr]) break;
            m_table_used[table][ptr] = 1;
            if (table != STBL) {
                if (m_song.ltable[table][ptr - 1] == 0xff) ptr = m_song.rtable[table][ptr - 1];
                else
                    ptr++;
            }
            else {
                break;
            }
        }
    }

    // Note whether a speed-table entry is zero, calculated (>= $80), or normal.
    void calc_speed_test(uint8_t pos) {
        if (!pos) {
            m_flags.no_zero_speed = false;
            return;
        }
        if (m_song.ltable[STBL][pos - 1] >= 0x80) m_flags.no_calculated_speed = false;
        else                                    m_flags.no_normal_speed     = false;
    }

    // True if this table segment is fully used and does not jump (or get jumped) outside itself.
    bool is_used_and_self_contained(int table, int start) const {
        int len = table_part_len(table, start - 1);
        int end = start + len - 1;
        if (len == 1) return false;
        for (int c = start; c <= end; c++)
            if (m_table_used[table][c] == 0) return false;
        if (m_song.rtable[table][end - 1] != 0) {
            if (m_song.rtable[table][end - 1] < start || m_song.rtable[table][end - 1] > end) return false;
        }
        for (int c = 1; c < start; c++)
            if (m_table_used[table][c] && m_song.ltable[table][c - 1] == 0xff &&
                m_song.rtable[table][c - 1] >= start && m_song.rtable[table][c - 1] <= end)
                return false;
        for (int c = end + 1; c <= MAX_TABLELEN; c++)
            if (m_table_used[table][c] && m_song.ltable[table][c - 1] == 0xff &&
                m_song.rtable[table][c - 1] >= start && m_song.rtable[table][c - 1] <= end)
                return false;
        return true;
    }

    // Drop duplicate table segments and remap pointers onto the first copy.
    void find_table_duplicates(int table) {
        auto const& ltable = m_song.ltable;
        auto const& rtable = m_song.rtable;
        if (table == STBL) {
            for (int c = 1; c <= MAX_TABLELEN; c++) {
                if (!m_table_used[table][c]) continue;
                for (int d = c + 1; d <= MAX_TABLELEN; d++) {
                    if (!m_table_used[table][d]) continue;
                    if (ltable[table][d - 1] == ltable[table][c - 1] &&
                        rtable[table][d - 1] == rtable[table][c - 1]) {
                        m_table_used[table][d] = 0;
                        for (int e = d; e <= MAX_TABLELEN; e++)
                            if (m_table_used[table][e]) m_table_map[table][e]--;
                        m_table_map[table][d] = m_table_map[table][c];
                    }
                }
            }
            return;
        }
        for (int c = 1; c <= MAX_TABLELEN; c++) {
            if (!is_used_and_self_contained(table, c)) continue;
            for (int d = c + table_part_len(table, c - 1); d <= MAX_TABLELEN;) {
                int len = table_part_len(table, d - 1);
                if (is_used_and_self_contained(table, d)) {
                    int e;
                    for (e = 0; e < len; e++) {
                        if (e < len - 1) {
                            if (ltable[table][d + e - 1] != ltable[table][c + e - 1] ||
                                rtable[table][d + e - 1] != rtable[table][c + e - 1])
                                break;
                        }
                        else {
                            if (ltable[table][d + e - 1] != ltable[table][c + e - 1]) break;
                            if (rtable[table][d + e - 1] == 0) {
                                if (rtable[table][c + e - 1] != 0) break;
                            }
                            else if ((rtable[table][d + e - 1] - d) != (rtable[table][c + e - 1] - c)) {
                                break;
                            }
                        }
                    }
                    if (e == len) {
                        for (e = 0; e < len; e++) m_table_used[table][d + e] = 0;
                        for (e = d; e < MAX_TABLELEN; e++)
                            if (m_table_used[table][e]) m_table_map[table][e] -= uint8_t(len);
                        for (e = 0; e < len; e++) m_table_map[table][d + e] = m_table_map[table][c + e];
                    }
                }
                d += len;
            }
        }
    }

    // Wrap assembled player bytes as BIN, PRG (2-byte load address), or PSID v2.
    std::vector<uint8_t> wrap_output(std::vector<uint8_t> const& packed, int multiplier) {
        uint16_t addr     = m_opt.player_addr & 0xff00; // GT: player always starts on a page
        bool     cia_stub = (multiplier > 1) || (multiplier == 0);

        // LDX #lo / STX $DC04 / LDX #hi / STX $DC05, then falls into jmp mt_init.
        uint8_t speed_code[10] = { 0xa2, 0x00, 0x8e, 0x04, 0xdc, 0xa2, 0x00, 0x8e, 0x05, 0xdc };
        if (cia_stub) {
            // PAL ~50Hz $4CC7, NTSC ~60Hz $42C6
            int speed_value = m_opt.ntsc ? 0x42c6u : 0x4cc7u;
            if (multiplier > 1) speed_value /= multiplier;
            else                speed_value *= 2;
            speed_code[1] = speed_value & 0xff;
            speed_code[6] = speed_value >> 8;
        }

        if (m_opt.format == ExportOptions::Format::Bin) return packed;
        if (m_opt.format == ExportOptions::Format::Prg) {
            std::vector<uint8_t> out;
            out.push_back(uint8_t(addr & 0xff));
            out.push_back(uint8_t(addr >> 8));
            out.insert(out.end(), packed.begin(), packed.end());
            return out;
        }

        // PSID v2, 0x7C-byte header. Multi-byte header fields are big-endian.
        std::vector<uint8_t> out;
        out.insert(out.end(), { 'P', 'S', 'I', 'D', 0x00, 0x02, 0x00, 0x7c }); // magic, version=2, dataOffset
        out.push_back(0); // loadAddress 0: C64 load address is first 2 bytes of data (little-endian)
        out.push_back(0);
        uint16_t init = cia_stub ? uint16_t(addr - 10) : addr; // CIA stub, else jmp mt_init
        out.push_back(uint8_t(init >> 8));
        out.push_back(uint8_t(init & 0xff));
        uint16_t play = uint16_t(addr + 3); // jmp mt_play
        out.push_back(uint8_t(play >> 8));
        out.push_back(uint8_t(play & 0xff));
        out.push_back(0);
        out.push_back(1); // songs
        out.push_back(0);
        out.push_back(1); // startSong
        // speed: $FFFFFFFF = CIA for all 32 songs; $00000000 = VIC raster (PAL 1x)
        uint8_t speed_byte = (m_opt.ntsc || multiplier > 1 || multiplier == 0) ? 0xff : 0x00;
        out.insert(out.end(), 4, speed_byte);

        out.insert(out.end(), m_song.song_name.begin(), m_song.song_name.end());
        out.insert(out.end(), m_song.author_name.begin(), m_song.author_name.end());
        out.insert(out.end(), m_song.copyright_name.begin(), m_song.copyright_name.end());

        // flags: bits 2-3 clock (PAL=01, NTSC=10), bits 4-5 SID (6581=01, 8580=10)
        out.push_back(0);
        uint8_t flags = m_opt.ntsc ? 8 : 4;
        flags |= (m_song.model == Model::MOS8580) ? 32 : 16;
        out.push_back(flags);
        out.insert(out.end(), 4, 0); // startPage, pageLength, reserved

        uint16_t load = cia_stub ? uint16_t(addr - 10) : addr;
        out.push_back(uint8_t(load & 0xff));
        out.push_back(uint8_t(load >> 8));
        if (cia_stub) out.insert(out.end(), speed_code, speed_code + 10);
        out.insert(out.end(), packed.begin(), packed.end());
        return out;
    }

    Song const&          m_song;
    ExportOptions const& m_opt;
    uint8_t              m_chn_used[MAX_CHN]{};
    uint8_t              m_patt_used[MAX_PATT]{};
    uint8_t              m_patt_map[MAX_PATT]{};
    uint8_t              m_instr_used[MAX_INSTR]{};
    uint8_t              m_instr_map[MAX_INSTR]{};
    uint8_t              m_table_used[MAX_TABLES][MAX_TABLELEN + 1]{};
    uint8_t              m_table_map[MAX_TABLES][MAX_TABLELEN + 1]{};
    PackFlags            m_flags;
    int                  m_channels          = 3;
    int                  m_fixed_params      = 1;
    int                  m_simple_pulse      = 1;
    int                  m_first_note        = MAX_NOTES - 1;
    int                  m_last_note         = 0;
    int                  m_pattern_last_note = 0;
    int                  m_patterns          = 0;
    int                  m_instruments       = 0;
    int                  m_num_legato        = 0;
    int                  m_num_no_hr         = 0;
    int                  m_num_normal        = 0;
    int                  m_trans_up_range    = 0;
    int                  m_trans_down_range  = 0;
};

} // namespace

// Used only by tests.
std::vector<uint8_t> pack_pattern(Pattern const& patt,
                                  uint8_t const  instr_map[MAX_INSTR],
                                  uint8_t const  table_map[MAX_TABLES][MAX_TABLELEN + 1],
                                  bool           strip_effects)
{
    PackFlags f;
    f.no_effects = strip_effects;
    return pack_pattern_raw(patt, instr_map, table_map, f);
}

std::vector<uint8_t> export_song(Song const& song, ExportOptions const& opt) {
    Song gt = song;
    gt.to_goattracker();
    Packer p{gt, opt};
    return p.build();
}

} // namespace gt
