#include "sid_export.hpp"
#include "gtsong.hpp"

#include <cctype>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <system_error>

namespace {

void usage() {
    fprintf(stdout,
        "Usage: sng2sid [options] <song.sng> <outfile>\n"
        "  outfile extension selects format: .sid .bin, otherwise .prg\n"
        "\n"
        "Options:\n"
        "  -A<hex>  hardrestart ADSR (default: from song)\n"
        "  -B<0|1>  buffered SID writes (default: 0)\n"
        "  -C<0|1>  zeropage ghost registers (default: 0)\n"
        "  -D<0|1>  sound effect support (default: 0)\n"
        "  -E<0|1>  volume-change support (default: 0)\n"
        "  -H<0|1>  store author info (default: 0)\n"
        "  -I<0|1>  playroutine optimizations (default: 1)\n"
        "  -J<0|1>  full buffering (default: 0)\n"
        "  -L<hex>  SID address (default: $D400)\n"
        "  -N       NTSC timing (default: PAL)\n"
        "  -O<0|1>  skip idle pulse-table work (default: 0)\n"
        "  -P       PAL timing\n"
        "  -R<0|1>  skip idle realtime-effect work (default: 0)\n"
        "  -S<n>    speed multiplier, 0 = 25Hz (default: from song)\n"
        "  -W<hex>  player address high byte (default: $10 = $1000)\n"
        "  -Z<hex>  zeropage address (default: $FC)\n"
        "  -?       show this help\n");
}

template<class T>
bool parse_int(char const* s, int base, T& dest) {
    T v{};
    auto [p, ec] = std::from_chars(s, s + std::strlen(s), v, base);
    if (ec != std::errc{} || *p) return false;
    dest = v;
    return true;
}

gt::ExportOptions::Format format_from_path(std::string const& path) {
    auto dot = path.find_last_of('.');
    if (dot == std::string::npos) return gt::ExportOptions::Format::Prg;
    std::string ext = path.substr(dot + 1);
    for (char& c : ext) c = char(tolower(unsigned(c)));
    if (ext == "sid") return gt::ExportOptions::Format::Sid;
    if (ext == "bin") return gt::ExportOptions::Format::Bin;
    return gt::ExportOptions::Format::Prg;
}

} // namespace

int main(int argc, char** argv) {
    gt::ExportOptions opt;
    char const* in_path = nullptr;
    char const* out_path = nullptr;

    for (int c = 1; c < argc; c++) {
        char const* a = argv[c];
        if (a[0] != '-') {
            if (!in_path) in_path = a;
            else if (!out_path) out_path = a;
            else {
                fprintf(stderr, "error: unexpected argument '%s'\n", a);
                usage();
                return 1;
            }
            continue;
        }
        char const* val = a + 2;
        switch (a[1]) {
        case '?': usage(); return 0;
        case 'A': parse_int(val, 16, opt.adparam_override); break;
        case 'L': parse_int(val, 16, opt.sid_addr); break;
        case 'S': parse_int(val, 10, opt.multiplier_override); break;
        case 'O': opt.optimize_pulse    = *val == '1'; break;
        case 'R': opt.optimize_realtime = *val == '1'; break;
        case 'N': opt.ntsc              = true; break;
        case 'P': opt.ntsc              = false; break;
        case 'B': opt.buffered          = *val == '1'; break;
        case 'D': opt.sound_effects     = *val == '1'; break;
        case 'E': opt.volume            = *val == '1'; break;
        case 'H': opt.author_info       = *val == '1'; break;
        case 'C': opt.zp_ghostregs      = *val == '1'; break;
        case 'I': opt.optimize          = *val == '1'; break;
        case 'J': opt.full_buffered     = *val == '1'; break;
        case 'W': {
            unsigned v = opt.player_addr >> 8;
            parse_int(val, 16, v);
            opt.player_addr = uint16_t(v << 8);
            break;
        }
        case 'Z': parse_int(val, 16, opt.zp_base); break;
        default:
            fprintf(stderr, "error: unknown option '%s'\n", argv[c]);
            usage();
            return 1;
        }
    }

    if (!in_path || !out_path) {
        usage();
        return 1;
    }
    opt.format = format_from_path(out_path);

    if (opt.multiplier_override > 16) opt.multiplier_override = 16;
    opt.player_addr &= 0xff00;
    opt.sid_addr &= 0xffff;

    try {
        gt::Song song;
        song.load(in_path);
        auto bytes = gt::export_song(song, opt);
        std::ofstream out(out_path, std::ios::binary);
        if (!out) {
            fprintf(stderr, "error: could not open output file '%s'.\n", out_path);
            return 1;
        }
        out.write(reinterpret_cast<char const*>(bytes.data()), std::streamsize(bytes.size()));
        fprintf(stdout, "sng2sid: wrote %s (%zu bytes)\n", out_path, bytes.size());
        return 0;
    } catch (std::exception const& e) {
        fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
}
