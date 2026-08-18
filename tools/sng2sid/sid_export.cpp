#include "sid_export.hpp"
#include "player_embed.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

extern "C" {
#include "assembler.h"
}

namespace gt {
namespace {

constexpr int MAX_NOTES     = 96;
constexpr int TYPE_OVERFLOW = 1;
constexpr int TYPE_JUMP     = 2;

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

// Pack editor pattern rows into playroutine bytes and record which effects were used.
int pack_pattern_raw(uint8_t*       dest,
                     uint8_t const* src,
                     int            rows,
                     uint8_t const* instr_map,
                     uint8_t const  table_map[MAX_TABLES][MAX_TABLELEN + 1],
                     PackFlags&     f) {
    uint8_t temp1[MAX_PATTROWS * 4];
    uint8_t temp2[512];
    uint8_t instr        = 0;
    int     command      = -1;
    int     data_byte    = -1;
    int     dest_size_im = 0;
    int     dest_size    = 0;

    for (int c = 0; c < rows; c++) {
        if (c && src[c * 4 + 1] && src[c * 4 + 1] == instr) {
            temp1[c * 4]     = src[c * 4];
            temp1[c * 4 + 1] = 0;
            temp1[c * 4 + 2] = src[c * 4 + 2];
            temp1[c * 4 + 3] = src[c * 4 + 3];
        }
        else {
            memcpy(&temp1[c * 4], &src[c * 4], 4);
            if (src[c * 4 + 1]) instr = src[c * 4 + 1];
        }
        switch (temp1[c * 4 + 2]) {
        case CMD_PORTAUP:
        case CMD_PORTADOWN:
            f.no_portamento  = false;
            temp1[c * 4 + 3] = table_map[STBL][temp1[c * 4 + 3]];
            break;
        case CMD_TONEPORTA:
            f.no_tone_porta  = false;
            temp1[c * 4 + 3] = table_map[STBL][temp1[c * 4 + 3]];
            break;
        case CMD_VIBRATO:
            f.no_vib         = false;
            temp1[c * 4 + 3] = table_map[STBL][temp1[c * 4 + 3]];
            break;
        case CMD_SETAD: f.no_set_ad = false; break;
        case CMD_SETSR: f.no_set_sr = false; break;
        case CMD_SETWAVE: f.no_set_wave = false; break;
        case CMD_SETWAVEPTR:
            f.no_set_wave_ptr = false;
            temp1[c * 4 + 3]  = table_map[WTBL][temp1[c * 4 + 3]];
            break;
        case CMD_SETPULSEPTR:
            f.no_set_pulse_ptr = false;
            f.no_pulse         = false;
            temp1[c * 4 + 3]   = table_map[PTBL][temp1[c * 4 + 3]];
            break;
        case CMD_SETFILTERPTR:
            f.no_set_filt_ptr = false;
            f.no_filter       = false;
            temp1[c * 4 + 3]  = table_map[FTBL][temp1[c * 4 + 3]];
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
            if (!f.author_info && temp1[c * 4 + 3] > 0x0f) {
                temp1[c * 4 + 2] = 0;
                temp1[c * 4 + 3] = 0;
            }
            break;
        case CMD_FUNKTEMPO:
            f.no_funk_tempo  = false;
            temp1[c * 4 + 3] = table_map[STBL][temp1[c * 4 + 3]];
            break;
        case CMD_SETTEMPO:
            if (temp1[c * 4 + 3] >= 0x80) f.no_channel_tempo = false;
            else
                f.no_global_tempo = false;
            if ((temp1[c * 4 + 3] & 0x7f) >= 3) temp1[c * 4 + 3]--;
            break;
        }
    }

    if (f.no_effects) {
        command   = 0;
        data_byte = 0;
    }

    for (int c = 0; c < rows; c++) {
        if (temp1[c * 4 + 1]) temp2[dest_size_im++] = instr_map[temp1[c * 4 + 1]];
        if (temp1[c * 4] == REST) {
            if (temp1[c * 4 + 2] != command || temp1[c * 4 + 3] != data_byte) {
                command               = temp1[c * 4 + 2];
                data_byte             = temp1[c * 4 + 3];
                temp2[dest_size_im++] = uint8_t(FXONLY + command);
                if (command) temp2[dest_size_im++] = uint8_t(data_byte);
            }
            else {
                temp2[dest_size_im++] = REST;
            }
        }
        else {
            if (temp1[c * 4 + 2] != command || temp1[c * 4 + 3] != data_byte) {
                command               = temp1[c * 4 + 2];
                data_byte             = temp1[c * 4 + 3];
                temp2[dest_size_im++] = uint8_t(FX + command);
                if (command) temp2[dest_size_im++] = uint8_t(data_byte);
            }
            temp2[dest_size_im++] = temp1[c * 4];
        }
    }

    for (int c = 0; c < dest_size_im;) {
        int pack_ok = 1;
        if (!c) pack_ok = 0;
        if (temp2[c] < FX) {
            dest[dest_size++] = temp2[c++];
            pack_ok           = 0;
        }
        if (temp2[c] >= FXONLY && temp2[c] < FIRSTNOTE) {
            int fx_num        = temp2[c] - FXONLY;
            dest[dest_size++] = temp2[c++];
            if (fx_num) dest[dest_size++] = temp2[c++];
            pack_ok = 0;
            continue;
        }
        if (temp2[c] < FXONLY) {
            int fx_num        = temp2[c] - FX;
            dest[dest_size++] = temp2[c++];
            if (fx_num) dest[dest_size++] = temp2[c++];
            pack_ok = 0;
        }
        if (temp2[c] != REST) pack_ok = 0;
        if (!pack_ok) {
            dest[dest_size++] = temp2[c++];
        }
        else {
            int d = c;
            while (d < dest_size_im && temp2[d] == REST) {
                d++;
                if (d - c == 64) break;
            }
            d -= c;
            if (d > 1) {
                dest[dest_size++] = uint8_t(-d);
                c += d;
            }
            else {
                dest[dest_size++] = temp2[c++];
            }
        }
    }
    if (dest_size > 256) return -1;
    if (dest_size < 256) dest[dest_size++] = 0x00;
    return dest_size;
}

struct Packer {
    uint8_t                                   chn_used[MAX_CHN]{};
    uint8_t                                   patt_used[MAX_PATT]{};
    uint8_t                                   patt_map[MAX_PATT]{};
    uint8_t                                   instr_used[MAX_INSTR]{};
    uint8_t                                   instr_map[MAX_INSTR]{};
    uint8_t                                   table_used[MAX_TABLES][MAX_TABLELEN + 1]{};
    uint8_t                                   table_map[MAX_TABLES][MAX_TABLELEN + 1]{};
    Array2<uint8_t, MAX_TABLES, MAX_TABLELEN> ltable{};
    Array2<uint8_t, MAX_TABLES, MAX_TABLELEN> rtable{};
    PackFlags                                 f;
    int                                       table_error       = 0;
    int                                       channels          = 3;
    int                                       fixed_params      = 1;
    int                                       simple_pulse      = 1;
    int                                       first_note        = MAX_NOTES - 1;
    int                                       last_note         = 0;
    int                                       pattern_last_note = 0;
    int                                       patterns          = 0;
    int                                       instruments       = 0;
    int                                       num_legato        = 0;
    int                                       num_no_hr         = 0;
    int                                       num_normal        = 0;
    int                                       trans_up_range    = 0;
    int                                       trans_down_range  = 0;

    // Rows from pos through the terminating $ff jump (speed table: always 1).
    int table_part_len(int num, int pos) const {
        if (pos < 0) return 0;
        if (num == STBL) return 1;
        int c;
        for (c = pos; c < MAX_TABLELEN; c++) {
            if (ltable[num][c] == 0xff) {
                c++;
                break;
            }
        }
        return c - pos;
    }

    // Mark table rows reachable from ptr; sets table_error on a jump-to-jump or overflow.
    void exec_table(int num, int ptr) {
        if (num != STBL && ptr && ptr <= MAX_TABLELEN) {
            if (ltable[num][ptr - 1] == 0xff) {
                table_error = TYPE_JUMP;
                return;
            }
        }
        for (;;) {
            if (!ptr) break;
            if (num != STBL && ptr > MAX_TABLELEN) {
                table_error = TYPE_OVERFLOW;
                break;
            }
            if (table_used[num][ptr]) break;
            table_used[num][ptr] = 1;
            if (num != STBL) {
                if (ltable[num][ptr - 1] == 0xff) ptr = rtable[num][ptr - 1];
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
            f.no_zero_speed = false;
            return;
        }
        if (ltable[STBL][pos - 1] >= 0x80) f.no_calculated_speed = false;
        else                               f.no_normal_speed     = false;
    }

    // True if this table segment is fully used and does not jump (or get jumped) outside itself.
    int is_used_and_self_contained(int num, int start) const {
        int len = table_part_len(num, start - 1);
        int end = start + len - 1;
        if (len == 1) return 0;
        for (int c = start; c <= end; c++)
            if (table_used[num][c] == 0) return 0;
        if (rtable[num][end - 1] != 0) {
            if (rtable[num][end - 1] < start || rtable[num][end - 1] > end) return 0;
        }
        for (int c = 1; c < start; c++)
            if (table_used[num][c] && ltable[num][c - 1] == 0xff && rtable[num][c - 1] >= start &&
                rtable[num][c - 1] <= end)
                return 0;
        for (int c = end + 1; c <= MAX_TABLELEN; c++)
            if (table_used[num][c] && ltable[num][c - 1] == 0xff && rtable[num][c - 1] >= start &&
                rtable[num][c - 1] <= end)
                return 0;
        return 1;
    }

    // Drop duplicate table segments and remap pointers onto the first copy.
    void find_table_duplicates(int num) {
        if (num == STBL) {
            for (int c = 1; c <= MAX_TABLELEN; c++) {
                if (!table_used[num][c]) continue;
                for (int d = c + 1; d <= MAX_TABLELEN; d++) {
                    if (!table_used[num][d]) continue;
                    if (ltable[num][d - 1] == ltable[num][c - 1] && rtable[num][d - 1] == rtable[num][c - 1]) {
                        table_used[num][d] = 0;
                        for (int e = d; e <= MAX_TABLELEN; e++)
                            if (table_used[num][e]) table_map[num][e]--;
                        table_map[num][d] = table_map[num][c];
                    }
                }
            }
            return;
        }
        for (int c = 1; c <= MAX_TABLELEN; c++) {
            if (!is_used_and_self_contained(num, c)) continue;
            for (int d = c + table_part_len(num, c - 1); d <= MAX_TABLELEN;) {
                int len = table_part_len(num, d - 1);
                if (is_used_and_self_contained(num, d)) {
                    int e;
                    for (e = 0; e < len; e++) {
                        if (e < len - 1) {
                            if (ltable[num][d + e - 1] != ltable[num][c + e - 1] ||
                                rtable[num][d + e - 1] != rtable[num][c + e - 1])
                                break;
                        }
                        else {
                            if (ltable[num][d + e - 1] != ltable[num][c + e - 1]) break;
                            if (rtable[num][d + e - 1] == 0) {
                                if (rtable[num][c + e - 1] != 0) break;
                            }
                            else if ((rtable[num][d + e - 1] - d) != (rtable[num][c + e - 1] - c)) {
                                break;
                            }
                        }
                    }
                    if (e == len) {
                        for (e = 0; e < len; e++) table_used[num][d + e] = 0;
                        for (e = d; e < MAX_TABLELEN; e++)
                            if (table_used[num][e]) table_map[num][e] -= uint8_t(len);
                        for (e = 0; e < len; e++) table_map[num][d + e] = table_map[num][c + e];
                    }
                }
                d += len;
            }
        }
    }

    // Flatten a pattern to note/instr/cmd/data bytes.
    void fill_pattern(Song const& song, int p, uint8_t* out) const {
        Pattern const& patt = song.patterns[p];
        for (int r = 0; r < patt.len; ++r) {
            PatternRow row = patt.rows[r];
            out[r * 4 + 0] = row.note;
            out[r * 4 + 1] = row.instr;
            out[r * 4 + 2] = row.command;
            out[r * 4 + 3] = row.data;
        }
    }

    // Walk the song: used channels/patterns/instruments/tables, and which player features to keep.
    void scan(Song const& song, ExportOptions const& opt) {
        ltable        = song.ltable;
        rtable        = song.rtable;
        f.author_info = opt.author_info;

        if (song.song_len <= 0) throw ExportError("no songs, no data to save");

        for (int d = 0; d < MAX_CHN; d++) {
            int trans = 0;
            for (int r = 0; r < song.song_len; r++) {
                OrderRow const& row = song.song_order[d][r];
                if (row.trans != trans) {
                    f.no_trans = false;
                    trans      = row.trans;
                    if (trans < 0) {
                        int nd = -trans;
                        if (nd > trans_down_range) trans_down_range = nd;
                    }
                    else if (trans > trans_up_range) {
                        trans_up_range = trans;
                    }
                }
                uint8_t num = row.pattnum;
                if (num >= MAX_PATT) throw ExportError("invalid pattern number in orderlist");
                patt_used[num]      = 1;
                Pattern const& patt = song.patterns[num];
                for (int k = 0; k < patt.len; k++) {
                    PatternRow const& pr = patt.rows[k];
                    if (pr.note != REST || pr.instr || pr.command) chn_used[d] = 1;
                }
            }
        }

        if (!chn_used[2]) channels = 2;
        if (!chn_used[1] && !chn_used[2]) channels = 1;

        instr_used[1] = 1;
        for (int c = 0; c < MAX_PATT; c++) {
            if (!patt_used[c]) continue;
            patt_map[c] = uint8_t(patterns++);
            uint8_t src[MAX_PATTROWS * 4];
            fill_pattern(song, c, src);
            Pattern const& patt = song.patterns[c];
            for (int d = 0; d < patt.len; d++) {
                table_error  = 0;
                uint8_t note = src[d * 4];
                uint8_t ins  = src[d * 4 + 1];
                uint8_t cmd  = src[d * 4 + 2];
                uint8_t data = src[d * 4 + 3];
                if (note == KEYOFF || note == KEYON) f.no_gate = false;
                if (ins) instr_used[ins] = 1;
                if (cmd) f.no_effects = false;
                if (cmd >= CMD_SETWAVEPTR && cmd <= CMD_SETFILTERPTR) exec_table(cmd - CMD_SETWAVEPTR, data);
                if (cmd >= CMD_PORTAUP && cmd <= CMD_VIBRATO) {
                    exec_table(STBL, data);
                    calc_speed_test(data);
                }
                if (cmd == CMD_FUNKTEMPO) {
                    exec_table(STBL, data);
                    f.no_funk_tempo   = false;
                    f.no_global_tempo = false;
                }
                if (cmd == CMD_SETTEMPO && (data & 0x7f) < 3) f.no_funk_tempo = false;
                if (note >= FIRSTNOTE && note <= LASTNOTE) {
                    int new_first = note - FIRSTNOTE - trans_down_range;
                    int new_last  = note - FIRSTNOTE + trans_up_range;
                    if (new_first < 0) new_first = 0;
                    if (new_last > MAX_NOTES - 1) new_last = MAX_NOTES - 1;
                    if (new_first < first_note) first_note = new_first;
                    if (new_last > last_note) {
                        pattern_last_note = new_last;
                        last_note         = new_last;
                    }
                    if (new_first > last_note) {
                        pattern_last_note = new_first;
                        last_note         = new_first;
                    }
                }
                if (table_error)
                    throw ExportError(table_error == TYPE_JUMP ? "table pointer points to a jump"
                                                               : "table execution overflows");
            }
        }

        for (int c = 0; c < MAX_INSTR; c++) {
            if (!instr_used[c]) continue;
            if (song.instruments[c].gatetimer & 0x40) num_legato++;
            else if (song.instruments[c].gatetimer & 0x80)
                num_no_hr++;
            else
                num_normal++;
            uint8_t fw = song.instruments[c].firstwave;
            if (!fw || fw >= 0xfe) f.no_first_wave_cmd = false;
        }
        int free_normal = 1;
        int free_no_hr  = free_normal + num_normal;
        int free_legato = free_no_hr + num_no_hr;
        for (int c = 0; c < MAX_INSTR; c++) {
            if (!instr_used[c]) continue;
            if (song.instruments[c].gatetimer & 0x40) instr_map[c] = uint8_t(free_legato++);
            else if (song.instruments[c].gatetimer & 0x80)
                instr_map[c] = uint8_t(free_no_hr++);
            else
                instr_map[c] = uint8_t(free_normal++);
            instruments++;
            for (int d = 0; d < MAX_TABLES; d++) {
                table_error = 0;
                uint8_t ptr = song.instruments[c].ptr[d];
                if (d == STBL && song.instruments[c].vibdelay == 0 &&
                    (ptr == 0 || (ltable[STBL][ptr - 1] == 0 && rtable[STBL][ptr - 1] == 0)))
                    continue;
                exec_table(d, ptr);
                if (d == STBL) calc_speed_test(ptr);
                if (table_error)
                    throw ExportError(table_error == TYPE_JUMP ? "table pointer points to a jump"
                                                               : "table execution overflows");
            }
        }

        for (int c = 0; c < MAX_TABLELEN; c++) {
            if (!table_used[WTBL][c + 1]) continue;
            if (ltable[WTBL][c] < WAVECMD || ltable[WTBL][c] > WAVELASTCMD) continue;
            int d       = -1;
            table_error = 0;
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
                f.no_pulse = false;
                break;
            case CMD_SETFILTERPTR:
                d           = FTBL;
                f.no_filter = false;
                break;
            case CMD_DONOTHING:
            case CMD_SETWAVEPTR:
            case CMD_FUNKTEMPO: throw ExportError("illegal wavetable command");
            }
            if (d != -1) exec_table(d, rtable[WTBL][c]);
            if (table_error)
                throw ExportError(table_error == TYPE_JUMP ? "table pointer points to a jump"
                                                           : "table execution overflows");
        }

        for (int c = 0; c < MAX_TABLES; c++) {
            int e = 1;
            for (int d = 0; d < MAX_TABLELEN; d++) {
                if (table_used[c][d + 1]) table_map[c][d + 1] = uint8_t(e++);
            }
        }
        for (int c = 0; c < MAX_TABLES; c++) find_table_duplicates(c);
    }

    // Emit packed song data as ASM, assemble it with the player, and wrap SID/PRG/BIN.
    std::vector<uint8_t> emit_and_assemble(Song const& song, ExportOptions const& opt) {
        int adparam    = opt.adparam_override >= 0 ? opt.adparam_override : song.adparam;
        int multiplier = opt.multiplier_override >= 0 ? opt.multiplier_override : song.multiplier;
        if (multiplier > 16) multiplier = 16;

        bool sfx      = opt.sound_effects;
        bool ghost_zp = opt.zp_ghostregs;
        bool full_buf = opt.full_buffered;
        bool buffered = opt.buffered || sfx || ghost_zp || full_buf;
        if (sfx || full_buf || ghost_zp) channels = 3;

        if (!opt.optimize) {
            fixed_params = 0;
            if (!num_legato) num_legato++;
            simple_pulse  = 0;
            first_note    = 0;
            last_note     = MAX_NOTES - 1;
            f             = PackFlags{};
            f.author_info = opt.author_info;
            f.no_effects = f.no_gate = f.no_filter = f.no_filter_mod = false;
            f.no_pulse = f.no_pulse_mod = f.no_wave_delay = f.no_wave_cmd = false;
            f.no_repeat = f.no_trans = false;
            f.no_portamento = f.no_tone_porta = f.no_vib = f.no_ins_vib = false;
            f.no_set_ad = f.no_set_sr = f.no_set_wave = false;
            f.no_set_wave_ptr = f.no_set_pulse_ptr = f.no_set_filt_ptr = false;
            f.no_set_filt_cutoff = f.no_set_filt_ctrl = f.no_set_master_vol = false;
            f.no_funk_tempo = f.no_global_tempo = f.no_channel_tempo = false;
            f.no_first_wave_cmd = f.no_calculated_speed = f.no_normal_speed = f.no_zero_speed = false;
        }

        std::vector<uint8_t> song_work;
        int                  song_offset[MAX_CHN]{};
        int                  song_size[MAX_CHN]{};
        for (int d = 0; d < MAX_CHN; d++) {
            song_offset[d] = int(song_work.size());
            int trans      = 0;
            int loop       = song.song_loop;
            for (int r = 0; r < song.song_len; r++) {
                OrderRow const& row = song.song_order[d][r];
                if (row.trans != trans) {
                    trans = row.trans;
                    song_work.push_back(uint8_t(trans + TRANSUP));
                    if (r < song.song_loop) ++loop;
                }
                song_work.push_back(patt_map[row.pattnum]);
            }
            song_work.push_back(LOOPSONG);
            song_work.push_back(uint8_t(loop));
            song_size[d] = int(song_work.size()) - song_offset[d];
            if (loop >= song_size[d] - 2) throw ExportError("illegal song restart position");
        }

        std::vector<uint8_t> patt_work;
        std::vector<int>     patt_offset;
        std::vector<int>     patt_size;
        uint8_t              patt_temp[512];
        uint8_t              src[MAX_PATTROWS * 4];
        for (int c = 0; c < MAX_PATT; c++) {
            if (!patt_used[c]) continue;
            fill_pattern(song, c, src);
            int result = pack_pattern_raw(patt_temp, src, song.patterns[c].len, instr_map, table_map, f);
            if (result < 0) throw ExportError("pattern too complex (over 256 bytes packed)");
            patt_offset.push_back(int(patt_work.size()));
            patt_size.push_back(result);
            patt_work.insert(patt_work.end(), patt_temp, patt_temp + result);
        }

        std::vector<uint8_t> instr_work(size_t(instruments) * 9, 0);
        for (int c = 1; c < MAX_INSTR; c++) {
            if (!instr_used[c]) continue;
            int               d             = instr_map[c] - 1;
            Instrument const& ins           = song.instruments[c];
            instr_work[d]                   = ins.ad;
            instr_work[d + instruments]     = ins.sr;
            instr_work[d + instruments * 2] = table_map[WTBL][ins.ptr[WTBL]];
            instr_work[d + instruments * 3] = table_map[PTBL][ins.ptr[PTBL]];
            instr_work[d + instruments * 4] = table_map[FTBL][ins.ptr[FTBL]];
            if (ins.vibdelay) {
                instr_work[d + instruments * 5] = table_map[STBL][ins.ptr[STBL]];
                instr_work[d + instruments * 6] = uint8_t(ins.vibdelay - 1);
            }
            instr_work[d + instruments * 7] = ins.gatetimer & 0x3f;
            instr_work[d + instruments * 8] = ins.firstwave;
            if (ins.ptr[STBL] && ins.vibdelay) {
                f.no_vib     = false;
                f.no_ins_vib = false;
            }
            if (ins.ptr[PTBL]) f.no_pulse = false;
            if (ins.ptr[FTBL]) f.no_filter = false;
            if (ins.gatetimer != song.instruments[1].gatetimer || ins.firstwave != song.instruments[1].firstwave)
                fixed_params = 0;
            if (!ins.firstwave || ins.firstwave >= 0xfe) fixed_params = 0;
        }

        if (multiplier > 1) {
            fixed_params = 0;
            num_legato++;
            num_no_hr++;
        }

        for (int c = 0; c < MAX_TABLELEN; c++) {
            if (!table_used[WTBL][c + 1]) continue;
            if (ltable[WTBL][c] >= WAVEDELAY && ltable[WTBL][c] <= WAVELASTDELAY) f.no_wave_delay = false;
            if (ltable[WTBL][c] >= WAVECMD && ltable[WTBL][c] <= WAVELASTCMD) {
                f.no_wave_cmd = false;
                f.no_effects  = false;
                switch (ltable[WTBL][c] - WAVECMD) {
                case CMD_PORTAUP:
                case CMD_PORTADOWN: f.no_portamento = false; break;
                case CMD_TONEPORTA: f.no_tone_porta = false; break;
                case CMD_VIBRATO: f.no_vib = false; break;
                case CMD_SETAD: f.no_set_ad = false; break;
                case CMD_SETSR: f.no_set_sr = false; break;
                case CMD_SETWAVE: f.no_set_wave = false; break;
                case CMD_SETPULSEPTR: f.no_set_pulse_ptr = false; break;
                case CMD_SETFILTERPTR: f.no_set_filt_ptr = false; break;
                case CMD_SETFILTERCUTOFF: f.no_set_filt_cutoff = false; break;
                case CMD_SETFILTERCTRL: f.no_set_filt_ctrl = false; break;
                case CMD_SETMASTERVOL: f.no_set_master_vol = false; break;
                }
            }
            if (ltable[WTBL][c] < WAVECMD) {
                if (rtable[WTBL][c] <= 0x80) {
                    int new_last = rtable[WTBL][c] + pattern_last_note;
                    if (new_last > MAX_NOTES - 1) new_last = MAX_NOTES - 1;
                    if (rtable[WTBL][c] >= 0x20) first_note = 0;
                    if (new_last > last_note) last_note = new_last;
                }
                else {
                    int nn = rtable[WTBL][c] & 0x7f;
                    if (nn > MAX_NOTES - 1) nn = MAX_NOTES - 1;
                    if (nn < first_note) first_note = nn;
                    if (nn > last_note) last_note = nn;
                }
            }
        }
        for (int c = 0; c < MAX_TABLELEN; c++) {
            if (!table_used[PTBL][c + 1]) continue;
            if (ltable[PTBL][c] >= 0x80 && ltable[PTBL][c] != 0xff) {
                if (rtable[PTBL][c] & 0xf) simple_pulse = 0;
            }
            if (ltable[PTBL][c] < 0x80) {
                f.no_pulse_mod = false;
                if (rtable[PTBL][c] & 0xf) simple_pulse = 0;
            }
        }
        for (int c = 0; c < MAX_TABLELEN; c++) {
            if (table_used[FTBL][c + 1] && ltable[FTBL][c] < 0x80) f.no_filter_mod = false;
        }

        if (last_note < first_note) last_note = first_note;
        if (first_note < 0) first_note = 0;
        if (!f.no_calculated_speed) last_note++;
        if (last_note > MAX_NOTES - 1) last_note = MAX_NOTES - 1;
        if (sfx) {
            first_note = 0;
            last_note  = MAX_NOTES - 1;
        }

        AsmSrc data;
        data.def("base", opt.player_addr);
        data.def("zpbase", opt.zp_base);
        data.def("SIDBASE", opt.sid_addr);
        data.def("SOUNDSUPPORT", sfx);
        data.def("VOLSUPPORT", opt.volume);
        data.def("BUFFEREDWRITES", buffered);
        data.def("GHOSTREGS", ghost_zp || full_buf);
        data.def("ZPGHOSTREGS", ghost_zp);
        data.def("FIXEDPARAMS", fixed_params);
        data.def("SIMPLEPULSE", simple_pulse);
        data.def("PULSEOPTIMIZATION", opt.optimize_pulse);
        data.def("REALTIMEOPTIMIZATION", opt.optimize_realtime);
        data.def("NOAUTHORINFO", !opt.author_info);
        data.def("NOEFFECTS", f.no_effects);
        data.def("NOGATE", f.no_gate);
        data.def("NOFILTER", f.no_filter);
        data.def("NOFILTERMOD", f.no_filter_mod);
        data.def("NOPULSE", f.no_pulse);
        data.def("NOPULSEMOD", f.no_pulse_mod);
        data.def("NOWAVEDELAY", f.no_wave_delay);
        data.def("NOWAVECMD", f.no_wave_cmd);
        data.def("NOREPEAT", f.no_repeat);
        data.def("NOTRANS", f.no_trans);
        data.def("NOPORTAMENTO", f.no_portamento);
        data.def("NOTONEPORTA", f.no_tone_porta);
        data.def("NOVIB", f.no_vib);
        data.def("NOINSTRVIB", f.no_ins_vib);
        data.def("NOSETAD", f.no_set_ad);
        data.def("NOSETSR", f.no_set_sr);
        data.def("NOSETWAVE", f.no_set_wave);
        data.def("NOSETWAVEPTR", f.no_set_wave_ptr);
        data.def("NOSETPULSEPTR", f.no_set_pulse_ptr);
        data.def("NOSETFILTPTR", f.no_set_filt_ptr);
        data.def("NOSETFILTCTRL", f.no_set_filt_ctrl);
        data.def("NOSETFILTCUTOFF", f.no_set_filt_cutoff);
        data.def("NOSETMASTERVOL", f.no_set_master_vol);
        data.def("NOFUNKTEMPO", f.no_funk_tempo);
        data.def("NOGLOBALTEMPO", f.no_global_tempo);
        data.def("NOCHANNELTEMPO", f.no_channel_tempo);
        data.def("NOFIRSTWAVECMD", f.no_first_wave_cmd);
        data.def("NOCALCULATEDSPEED", f.no_calculated_speed);
        data.def("NONORMALSPEED", f.no_normal_speed);
        data.def("NOZEROSPEED", f.no_zero_speed);
        data.def("NUMCHANNELS", channels);
        data.def("NUMSONGS", 1);
        data.def("FIRSTNOTE", first_note);
        data.def("FIRSTNOHRINSTR", num_normal + 1);
        data.def("FIRSTLEGATOINSTR", num_normal + num_no_hr + 1);
        data.def("NUMHRINSTR", num_normal);
        data.def("NUMNOHRINSTR", num_no_hr);
        data.def("NUMLEGATOINSTR", num_legato);
        data.def("ADPARAM", (adparam >> 8) & 0xff);
        data.def("SRPARAM", adparam & 0xff);
        if (song.instruments[MAX_INSTR - 1].ad >= 2 && !song.instruments[MAX_INSTR - 1].ptr[WTBL]) {
            data.def("DEFAULTTEMPO", song.instruments[MAX_INSTR - 1].ad - 1);
        }
        else {
            data.def("DEFAULTTEMPO", multiplier ? (multiplier * 6 - 1) : 5);
        }
        if (fixed_params) {
            data.def("FIRSTWAVEPARAM", song.instruments[1].firstwave);
            data.def("GATETIMERPARAM", song.instruments[1].gatetimer & 0x3f);
        }

        std::string player = load_player_source(adparam >= 0xf000);
        data.player(player);
        data.label("mt_freqtbllo");
        data.bytes(&FREQ_TBL_LO[first_note], last_note - first_note + 1);
        data.label("mt_freqtblhi");
        data.bytes(&FREQ_TBL_HI[first_note], last_note - first_note + 1);

        data.label("mt_songtbllo");
        for (int c = 0; c < 3; c++) data.addr_lo("mt_song" + std::to_string(c));
        data.label("mt_songtblhi");
        for (int c = 0; c < 3; c++) data.addr_hi("mt_song" + std::to_string(c));

        data.label("mt_patttbllo");
        for (int c = 0; c < patterns; c++) data.addr_lo("mt_patt" + std::to_string(c));
        data.label("mt_patttblhi");
        for (int c = 0; c < patterns; c++) data.addr_hi("mt_patt" + std::to_string(c));

        data.label("mt_insad");
        data.bytes(&instr_work[0], instruments);
        data.label("mt_inssr");
        data.bytes(&instr_work[size_t(instruments)], instruments);
        data.label("mt_inswaveptr");
        data.bytes(&instr_work[size_t(instruments) * 2], instruments);
        if (!f.no_pulse) {
            data.label("mt_inspulseptr");
            data.bytes(&instr_work[size_t(instruments) * 3], instruments);
        }
        if (!f.no_filter) {
            data.label("mt_insfiltptr");
            data.bytes(&instr_work[size_t(instruments) * 4], instruments);
        }
        if (!f.no_ins_vib) {
            data.label("mt_insvibparam");
            data.bytes(&instr_work[size_t(instruments) * 5], instruments);
            data.label("mt_insvibdelay");
            data.bytes(&instr_work[size_t(instruments) * 6], instruments);
        }
        if (!fixed_params) {
            data.label("mt_insgatetimer");
            data.bytes(&instr_work[size_t(instruments) * 7], instruments);
            data.label("mt_insfirstwave");
            data.bytes(&instr_work[size_t(instruments) * 8], instruments);
        }

        bool speed_extra = !f.no_vib || !f.no_funk_tempo || !f.no_portamento || !f.no_tone_porta;
        for (int c = 0; c < MAX_TABLES; c++) {
            if (c == PTBL && f.no_pulse) continue;
            if (c == FTBL && f.no_filter) continue;
            if (c == STBL && speed_extra) data.byte(0);
            data.label(TABLE_LEFT_NAME[c]);
            for (int d = 0; d < MAX_TABLELEN; d++) {
                if (!table_used[c][d + 1]) continue;
                switch (c) {
                case WTBL: {
                    uint8_t wave = ltable[c][d];
                    if (ltable[c][d] >= WAVESILENT && ltable[c][d] <= WAVELASTSILENT) wave &= 0xf;
                    if (ltable[c][d] > WAVELASTDELAY && ltable[c][d] <= WAVELASTSILENT && !f.no_wave_delay)
                        wave += 0x10;
                    data.byte(wave);
                    break;
                }
                case PTBL:
                    if (simple_pulse && ltable[c][d] != 0xff && ltable[c][d] > 0x80) data.byte(0x80);
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
                if (!table_used[c][d + 1]) continue;
                if (ltable[c][d] != 0xff || c == STBL) {
                    switch (c) {
                    case WTBL:
                        if (ltable[c][d] >= WAVECMD && ltable[c][d] <= WAVELASTCMD) {
                            switch (ltable[c][d] - WAVECMD) {
                            case CMD_PORTAUP:
                            case CMD_PORTADOWN:
                            case CMD_TONEPORTA:
                            case CMD_VIBRATO: data.byte(table_map[STBL][rtable[c][d]]); break;
                            case CMD_SETPULSEPTR: data.byte(table_map[PTBL][rtable[c][d]]); break;
                            case CMD_SETFILTERPTR: data.byte(table_map[FTBL][rtable[c][d]]); break;
                            default: data.byte(rtable[c][d]); break;
                            }
                        }
                        else {
                            data.byte(rtable[c][d] ^ 0x80);
                        }
                        break;
                    case PTBL:
                        if (simple_pulse) {
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
                    data.byte(table_map[c][rtable[c][d]]);
                }
            }
        }

        for (int d = 0; d < MAX_CHN; d++) {
            std::string n = "mt_song" + std::to_string(d);
            data.label(n.c_str());
            data.bytes(&song_work[size_t(song_offset[d])], song_size[d]);
        }
        for (int c = 0; c < patterns; c++) {
            std::string n = "mt_patt" + std::to_string(c);
            data.label(n.c_str());
            data.bytes(&patt_work[size_t(patt_offset[c])], patt_size[c]);
        }

        AssembleResult assembled = data.assemble();
        if (opt.author_info) {
            size_t off = 32;
            if (assembled.bytes.size() > off + 32) {
                for (int c = 0; c < 32; c++) {
                    char ch                          = song.author_name[c];
                    assembled.bytes[off + size_t(c)] = ch ? uint8_t(ch) : 0x20;
                }
            }
        }
        return wrap_output(assembled.bytes, song, opt, multiplier);
    }

    // Wrap assembled player bytes as BIN, PRG (load address), or PSID v2 (CIA stub if needed).
    std::vector<uint8_t> wrap_output(std::vector<uint8_t> const& packed,
                                     Song const&                 song,
                                     ExportOptions const&        opt,
                                     int                         multiplier)
    {
        uint16_t addr           = opt.player_addr & 0xff00;
        bool     cia_stub       = (multiplier > 1) || (multiplier == 0);
        uint8_t  speed_code[10] = { 0xa2, 0x00, 0x8e, 0x04, 0xdc, 0xa2, 0x00, 0x8e, 0x05, 0xdc };
        if (cia_stub) {
            unsigned speed_value;
            if (multiplier) speed_value = (opt.ntsc ? 0x42c6u : 0x4cc7u) / unsigned(multiplier);
            else
                speed_value = (opt.ntsc ? 0x42c6u : 0x4cc7u) * 2;
            speed_code[1] = uint8_t(speed_value & 0xff);
            speed_code[6] = uint8_t(speed_value >> 8);
        }

        if (opt.format == ExportOptions::Format::Bin) return packed;
        if (opt.format == ExportOptions::Format::Prg) {
            std::vector<uint8_t> out;
            out.push_back(uint8_t(addr & 0xff));
            out.push_back(uint8_t(addr >> 8));
            out.insert(out.end(), packed.begin(), packed.end());
            return out;
        }

        std::vector<uint8_t> out;
        uint8_t              ident[] = { 'P', 'S', 'I', 'D', 0x00, 0x02, 0x00, 0x7c };
        out.insert(out.end(), ident, ident + 8);
        out.push_back(0);
        out.push_back(0);
        uint16_t init = cia_stub ? uint16_t(addr - 10) : addr;
        out.push_back(uint8_t(init >> 8));
        out.push_back(uint8_t(init & 0xff));
        uint16_t play = uint16_t(addr + 3);
        out.push_back(uint8_t(play >> 8));
        out.push_back(uint8_t(play & 0xff));
        out.push_back(0);
        out.push_back(1);
        out.push_back(0);
        out.push_back(1);
        uint8_t speed_byte = (opt.ntsc || multiplier > 1 || multiplier == 0) ? 0xff : 0x00;
        out.insert(out.end(), 4, speed_byte);

        auto put32 = [&](std::array<char, MAX_STR> const& s) {
            for (int i = 0; i < MAX_STR; i++) out.push_back(uint8_t(s[i]));
        };
        put32(song.song_name);
        put32(song.author_name);
        put32(song.copyright_name);

        out.push_back(0);
        uint8_t flags = opt.ntsc ? 8 : 4;
        flags |= (song.model == Model::MOS8580) ? 32 : 16;
        out.push_back(flags);
        out.insert(out.end(), 4, uint8_t(0));

        uint16_t load = cia_stub ? uint16_t(addr - 10) : addr;
        out.push_back(uint8_t(load & 0xff));
        out.push_back(uint8_t(load >> 8));
        if (cia_stub) out.insert(out.end(), speed_code, speed_code + 10);
        out.insert(out.end(), packed.begin(), packed.end());
        return out;
    }
};

} // namespace

// Embedded player.asm, or altplayer.asm when hardrestart uses the alternate ADSR order.
std::string load_player_source(bool alt_player) {
    if (alt_player) return { ALTPLAYER_ASM, sizeof(ALTPLAYER_ASM) };
    return { PLAYER_ASM, sizeof(PLAYER_ASM) };
}

AssembleResult assemble_source(std::string const& source) {
    AsmSrc src;
    src.player(source);
    return src.assemble();
}

// Pack editor rows (note, instr, cmd, data repeating) into playroutine bytes.
std::vector<uint8_t> pack_pattern(uint8_t const* src,
                                  int            rows,
                                  uint8_t const  instr_map[MAX_INSTR],
                                  uint8_t const  table_map[MAX_TABLES][MAX_TABLELEN + 1],
                                  bool           strip_effects) {
    PackFlags f;
    f.no_effects = strip_effects;
    uint8_t dest[512];
    int     n = pack_pattern_raw(dest, src, rows, instr_map, table_map, f);
    if (n < 0) throw ExportError("pattern too complex");
    return { dest, dest + n };
}

// Pack a song and assemble a SID, PRG, or BIN.
std::vector<uint8_t> export_song(Song const& song, ExportOptions const& opt) {
    Song gt = song;
    gt.to_goattracker();
    Packer p;
    p.scan(gt, opt);
    return p.emit_and_assemble(gt, opt);
}

} // namespace gt
