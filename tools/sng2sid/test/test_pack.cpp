#include "sid_export.hpp"
#include "gtsong.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

namespace gt {
std::vector<uint8_t> pack_pattern(Pattern const& patt,
                                  uint8_t const  instr_map[MAX_INSTR],
                                  uint8_t const  table_map[MAX_TABLES][MAX_TABLELEN + 1],
                                  bool           strip_effects);
}

namespace {

int g_fails = 0;

void expect_eq(char const* name, std::vector<uint8_t> const& got, std::vector<uint8_t> const& want) {
    if (got == want) {
        fprintf(stdout, "OK %s\n", name);
        return;
    }
    fprintf(stderr, "FAIL %s: got %zu bytes, want %zu\n", name, got.size(), want.size());
    size_t n = std::min(got.size(), want.size());
    for (size_t i = 0; i < n; i++) {
        if (got[i] != want[i]) {
            fprintf(stderr, "  first diff +%zu: got %02x want %02x\n", i, got[i], want[i]);
            break;
        }
    }
    g_fails++;
}

uint8_t instr_map[gt::MAX_INSTR];
uint8_t table_map[gt::MAX_TABLES][gt::MAX_TABLELEN + 1];

void identity_maps() {
    memset(instr_map, 0, sizeof instr_map);
    memset(table_map, 0, sizeof table_map);
    for (int i = 0; i < gt::MAX_INSTR; i++) instr_map[i] = uint8_t(i);
    for (int t = 0; t < gt::MAX_TABLES; t++)
        for (int i = 0; i <= gt::MAX_TABLELEN; i++) table_map[t][i] = uint8_t(i);
}

std::vector<uint8_t> pack_rows(std::vector<gt::PatternRow> const& rows, bool strip) {
    gt::Pattern patt;
    patt.len = int(rows.size());
    std::copy(rows.begin(), rows.end(), patt.rows.begin());
    return gt::pack_pattern(patt, instr_map, table_map, strip);
}

} // namespace

int main() {
    identity_maps();

    // one rest, no effects stripped: REST + end
    expect_eq("rest1", pack_rows({ { gt::REST, 0, 0, 0 } }, true), { gt::REST, 0x00 });

    // three rests: first unpacked, next two packed as -2
    expect_eq("rest3", pack_rows({ { gt::REST, 0, 0, 0 }, { gt::REST, 0, 0, 0 }, { gt::REST, 0, 0, 0 } }, true),
              { gt::REST, uint8_t(-2), 0x00 });

    // plain note
    expect_eq("note", pack_rows({ { gt::FIRSTNOTE, 0, 0, 0 } }, true), { gt::FIRSTNOTE, 0x00 });

    // tempo 6 becomes 5 (playroutine stores tempo-1); not stripped so FX prefix
    expect_eq("tempo", pack_rows({ { gt::FIRSTNOTE, 0, gt::CMD_SETTEMPO, 6 } }, false),
              { uint8_t(gt::FX + gt::CMD_SETTEMPO), 5, gt::FIRSTNOTE, 0x00 });

    // duplicate instrument on second row is dropped
    expect_eq("instrdup", pack_rows({ { gt::FIRSTNOTE, 1, 0, 0 }, { gt::FIRSTNOTE + 1, 1, 0, 0 } }, true),
              { 1, gt::FIRSTNOTE, uint8_t(gt::FIRSTNOTE + 1), 0x00 });

    {
        gt::Song song;
        song.clear();
        song.instruments[3].ptr[gt::PTBL] = 0x0a;
        song.patterns[0].len = 1;
        song.patterns[0].rows[0] = { gt::REST, 0, gt::CMD_SETPULSEPTR, 3 };
        song.ltable[gt::WTBL][0] = uint8_t(gt::WAVECMD + gt::CMD_SETFILTERPTR);
        song.rtable[gt::WTBL][0] = 3;
        song.instruments[3].ptr[gt::FTBL] = 0x0c;
        try {
            song.to_goattracker();
            if (song.mode != gt::Mode::GoatTracker || song.patterns[0].rows[0].data != 0x0a ||
                song.rtable[gt::WTBL][0] != 0x0c) {
                fprintf(stderr, "FAIL to_goattracker remap\n");
                g_fails++;
            } else {
                fprintf(stdout, "OK to_goattracker\n");
            }
            song.to_goattracker();
            if (song.patterns[0].rows[0].data != 0x0a) {
                fprintf(stderr, "FAIL to_goattracker not idempotent\n");
                g_fails++;
            }
        } catch (std::exception const& e) {
            fprintf(stderr, "FAIL to_goattracker: %s\n", e.what());
            g_fails++;
        }
    }

    try {
        gt::Song song;
        song.clear();
        song.song_len = 1;
        song.song_order[0][0] = { 0, 0 };
        song.song_order[1][0] = { 0, 1 };
        song.song_order[2][0] = { 0, 2 };
        song.patterns[0].len = 1;
        song.patterns[0].rows[0] = { gt::FIRSTNOTE, 1, 0, 0 };
        song.instruments[1].ad = 0x13;
        song.instruments[1].sr = 0x37;
        song.instruments[1].firstwave = 0x09;
        song.instruments[1].gatetimer = 2;
        memcpy(song.song_name.data(), "test", 4);
        auto sid = gt::export_song(song, {});
        if (sid.size() < 0x7c + 8 || memcmp(sid.data(), "PSID", 4) != 0) {
            fprintf(stderr, "FAIL export: not a PSID (%zu bytes)\n", sid.size());
            g_fails++;
        } else if (sid[0x7c + 2] != 0x4c || sid[0x7c + 5] != 0x4c) {
            fprintf(stderr, "FAIL export: payload missing jmp init/play\n");
            g_fails++;
        } else {
            // One used instrument: mt_insad and mt_inssr are consecutive bytes.
            bool adsr = false;
            for (size_t i = 0x7c; i + 1 < sid.size(); i++) {
                if (sid[i] == 0x13 && sid[i + 1] == 0x37) {
                    adsr = true;
                    break;
                }
            }
            if (!adsr) {
                fprintf(stderr, "FAIL export: instrument AD/SR tables missing\n");
                g_fails++;
            } else {
                fprintf(stdout, "OK export_song PSID (%zu bytes)\n", sid.size());
            }
        }

        gt::ExportOptions prg;
        prg.format = gt::ExportOptions::Format::Prg;
        auto p = gt::export_song(song, prg);
        if (p.size() < 8 || p[0] != 0x00 || p[1] != 0x10 || p[2] != 0x4c) {
            fprintf(stderr, "FAIL prg wrap\n");
            g_fails++;
        } else {
            fprintf(stdout, "OK export_song PRG (%zu bytes)\n", p.size());
        }

        gt::ExportOptions bin;
        bin.format = gt::ExportOptions::Format::Bin;
        auto b = gt::export_song(song, bin);
        if (b.size() < 6 || b[0] != 0x4c) {
            fprintf(stderr, "FAIL bin wrap\n");
            g_fails++;
        } else {
            fprintf(stdout, "OK export_song BIN (%zu bytes)\n", b.size());
        }
    } catch (std::exception const& e) {
        fprintf(stderr, "FAIL export_song: %s\n", e.what());
        g_fails++;
    }

    if (g_fails) {
        fprintf(stderr, "%d pack test(s) failed\n", g_fails);
        return 1;
    }
    fprintf(stdout, "all pack tests passed\n");
    return 0;
}
