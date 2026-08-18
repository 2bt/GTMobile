#include "sid_export.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace {

int g_fails = 0;

void fail(char const* msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fails++;
}

std::map<std::string, int> parse_defines(char const* text) {
    std::map<std::string, int> out;
    char const* p = text;
    while (*p) {
        while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
        if (!*p) break;
        char name[64];
        int value = 1;
        int n = 0;
        if (sscanf(p, "%63[^=]=%d%n", name, &value, &n) >= 2) {
            out[name] = value;
            p += n;
        } else {
            break;
        }
        if (*p == '\n') p++;
    }
    return out;
}

char const* kFull = R"(
base=4096
zpbase=252
SIDBASE=54272
SOUNDSUPPORT=0
VOLSUPPORT=0
BUFFEREDWRITES=0
GHOSTREGS=0
ZPGHOSTREGS=0
FIXEDPARAMS=1
SIMPLEPULSE=1
PULSEOPTIMIZATION=1
REALTIMEOPTIMIZATION=1
NOAUTHORINFO=1
NOEFFECTS=0
NOGATE=0
NOFILTER=0
NOFILTERMOD=0
NOPULSE=0
NOPULSEMOD=0
NOWAVEDELAY=0
NOWAVECMD=0
NOREPEAT=0
NOTRANS=0
NOPORTAMENTO=0
NOTONEPORTA=0
NOVIB=0
NOINSTRVIB=0
NOSETAD=0
NOSETSR=0
NOSETWAVE=0
NOSETWAVEPTR=0
NOSETPULSEPTR=0
NOSETFILTPTR=0
NOSETFILTCTRL=0
NOSETFILTCUTOFF=0
NOSETMASTERVOL=0
NOFUNKTEMPO=0
NOGLOBALTEMPO=0
NOCHANNELTEMPO=0
NOFIRSTWAVECMD=0
NOCALCULATEDSPEED=0
NONORMALSPEED=0
NOZEROSPEED=0
NUMCHANNELS=3
NUMSONGS=1
FIRSTNOTE=0
FIRSTNOHRINSTR=2
FIRSTLEGATOINSTR=3
NUMHRINSTR=1
NUMNOHRINSTR=1
NUMLEGATOINSTR=1
ADPARAM=15
SRPARAM=0
DEFAULTTEMPO=5
FIRSTWAVEPARAM=9
GATETIMERPARAM=2
)";

char const* kMinimal = R"(
base=4096
zpbase=252
SIDBASE=54272
SOUNDSUPPORT=0
VOLSUPPORT=0
BUFFEREDWRITES=0
GHOSTREGS=0
ZPGHOSTREGS=0
FIXEDPARAMS=1
SIMPLEPULSE=1
PULSEOPTIMIZATION=1
REALTIMEOPTIMIZATION=1
NOAUTHORINFO=1
NOEFFECTS=1
NOGATE=1
NOFILTER=1
NOFILTERMOD=1
NOPULSE=1
NOPULSEMOD=1
NOWAVEDELAY=1
NOWAVECMD=1
NOREPEAT=1
NOTRANS=1
NOPORTAMENTO=1
NOTONEPORTA=1
NOVIB=1
NOINSTRVIB=1
NOSETAD=1
NOSETSR=1
NOSETWAVE=1
NOSETWAVEPTR=1
NOSETPULSEPTR=1
NOSETFILTPTR=1
NOSETFILTCTRL=1
NOSETFILTCUTOFF=1
NOSETMASTERVOL=1
NOFUNKTEMPO=1
NOGLOBALTEMPO=1
NOCHANNELTEMPO=1
NOFIRSTWAVECMD=1
NOCALCULATEDSPEED=1
NONORMALSPEED=1
NOZEROSPEED=1
NUMCHANNELS=3
NUMSONGS=1
FIRSTNOTE=0
FIRSTNOHRINSTR=2
FIRSTLEGATOINSTR=3
NUMHRINSTR=1
NUMNOHRINSTR=1
NUMLEGATOINSTR=1
ADPARAM=15
SRPARAM=0
DEFAULTTEMPO=5
FIRSTWAVEPARAM=9
GATETIMERPARAM=2
)";

std::string stubs() {
    return R"(
mt_freqtbllo: !byte 0
mt_freqtblhi: !byte 0
mt_songtbllo: !byte <mt_song0, <mt_song1, <mt_song2
mt_songtblhi: !byte >mt_song0, >mt_song1, >mt_song2
mt_patttbllo: !byte <mt_patt0
mt_patttblhi: !byte >mt_patt0
mt_insad: !byte 0
mt_inssr: !byte 0
mt_inswaveptr: !byte 0
mt_inspulseptr: !byte 0
mt_insfiltptr: !byte 0
mt_insvibparam: !byte 0
mt_insvibdelay: !byte 0
mt_insgatetimer: !byte 0
mt_insfirstwave: !byte 0
mt_wavetbl: !byte 0
mt_notetbl: !byte 0
mt_pulsetimetbl: !byte 0
mt_pulsespdtbl: !byte 0
mt_filttimetbl: !byte 0
mt_filtspdtbl: !byte 0
mt_speedlefttbl: !byte 0
mt_speedrighttbl: !byte 0
mt_song0: !byte $ff, 0
mt_song1: !byte $ff, 0
mt_song2: !byte $ff, 0
mt_patt0: !byte 0
)";
}

std::string golden_path(char const* name) {
#ifndef SNG2SID_DATADIR
#define SNG2SID_DATADIR "."
#endif
    return std::string(SNG2SID_DATADIR) + "/test/player/golden/" + name + ".bin";
}

void check_jmps(char const* name, std::vector<uint8_t> const& b, bool sfx, bool vol) {
    if (b.size() < 6 || b[0] != 0x4c || b[3] != 0x4c) {
        fail((std::string(name) + ": missing jmp mt_init / jmp mt_play").c_str());
        return;
    }
    size_t off = 6;
    if (sfx) {
        if (b.size() < off + 3 || b[off] != 0x4c)
            fail((std::string(name) + ": missing jmp mt_playsfx").c_str());
        off += 3;
    }
    if (vol) {
        if (b.size() < off + 3 || b[off] != 0x4c)
            fail((std::string(name) + ": missing jmp mt_setmastervol").c_str());
    }
}

void run_case(char const* name, std::map<std::string, int> defs, bool alt,
              bool sfx, bool vol) {
    try {
        std::string src;
        for (auto const& kv : defs) {
            char buf[80];
            snprintf(buf, sizeof buf, "%s = %d\n", kv.first.c_str(), kv.second);
            src += buf;
        }
        src += gt::load_player_source(alt);
        src += stubs();
        auto r = gt::assemble_source(src);
        check_jmps(name, r.bytes, sfx, vol);

        std::string gpath = golden_path(name);
        if (const char* w = getenv("SNG2SID_WRITE_GOLDENS"); w && w[0] == '1') {
            std::ofstream out(gpath, std::ios::binary);
            out.write(reinterpret_cast<char const*>(r.bytes.data()), std::streamsize(r.bytes.size()));
            fprintf(stdout, "wrote golden %s (%zu bytes)\n", gpath.c_str(), r.bytes.size());
            return;
        }
        std::ifstream in(gpath, std::ios::binary);
        if (!in) {
            fprintf(stderr, "FAIL: %s missing golden %s\n", name, gpath.c_str());
            g_fails++;
            return;
        }
        std::vector<uint8_t> golden((std::istreambuf_iterator<char>(in)), {});
        if (golden.size() != r.bytes.size() || memcmp(golden.data(), r.bytes.data(), r.bytes.size()) != 0) {
            size_t n = std::min(golden.size(), r.bytes.size());
            size_t i = 0;
            for (; i < n; i++) if (golden[i] != r.bytes[i]) break;
            fprintf(stderr, "FAIL: %s mismatch at +%zu (got %zu bytes, golden %zu)\n",
                    name, i, r.bytes.size(), golden.size());
            for (size_t k = i; k < i + 16 && k < n; k++)
                fprintf(stderr, "  %04zx: got %02x golden %02x\n", k, r.bytes[k], golden[k]);
            g_fails++;
        } else {
            fprintf(stdout, "OK %s (%zu bytes)\n", name, r.bytes.size());
        }
    } catch (std::exception const& e) {
        fprintf(stderr, "FAIL: %s assemble: %s\n", name, e.what());
        g_fails++;
    }
}

std::map<std::string, int> with(char const* base, std::map<std::string, int> extra) {
    auto m = parse_defines(base);
    for (auto const& kv : extra) m[kv.first] = kv.second;
    return m;
}

} // namespace

int main() {
    run_case("full", parse_defines(kFull), false, false, false);
    run_case("minimal", parse_defines(kMinimal), false, false, false);
    run_case("buffered", with(kFull, {{"BUFFEREDWRITES", 1}}), false, false, false);
    run_case("volume", with(kFull, {{"VOLSUPPORT", 1}}), false, false, true);
    run_case("sfx", with(kFull, {{"SOUNDSUPPORT", 1}, {"BUFFEREDWRITES", 1}, {"NUMCHANNELS", 3}}),
             false, true, false);
    run_case("ghost_zp", with(kFull, {{"GHOSTREGS", 1}, {"ZPGHOSTREGS", 1}, {"BUFFEREDWRITES", 1}, {"zpbase", 2}}),
             false, false, false);
    run_case("ghost_abs", with(kFull, {{"GHOSTREGS", 1}, {"ZPGHOSTREGS", 0}, {"BUFFEREDWRITES", 1}}),
             false, false, false);
    run_case("author", with(kFull, {{"NOAUTHORINFO", 0}}), false, false, false);
    run_case("ch1", with(kFull, {{"NUMCHANNELS", 1}}), false, false, false);
    run_case("ch2", with(kFull, {{"NUMCHANNELS", 2}}), false, false, false);
    run_case("unfixed", with(kFull, {{"FIXEDPARAMS", 0}, {"SIMPLEPULSE", 0}}), false, false, false);
    run_case("alt_full", parse_defines(kFull), true, false, false);
    run_case("pulse_only", with(kMinimal, {{"NOPULSE", 0}, {"NOPULSEMOD", 0}, {"NOSETPULSEPTR", 0}}),
             false, false, false);
    run_case("filter_only", with(kMinimal, {{"NOFILTER", 0}, {"NOFILTERMOD", 0}, {"NOSETFILTPTR", 0}}),
             false, false, false);
    run_case("vib_noporta", with(kMinimal, {{"NOVIB", 0}, {"NOINSTRVIB", 0}, {"NOEFFECTS", 0}}),
             false, false, false);
    run_case("wavecmd", with(kMinimal, {{"NOWAVECMD", 0}, {"NOEFFECTS", 0}}),
             false, false, false);
    run_case("funktempo", with(kMinimal, {{"NOFUNKTEMPO", 0}, {"NOGLOBALTEMPO", 0}}),
             false, false, false);
    run_case("no_gate", with(kMinimal, {{"NOGATE", 0}}), false, false, false);

    if (g_fails) {
        fprintf(stderr, "%d player test(s) failed\n", g_fails);
        return 1;
    }
    fprintf(stdout, "all player tests passed\n");
    return 0;
}
