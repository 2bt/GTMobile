#include "sid_export.hpp"
#include "gtsong.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

namespace {

void usage() {
    fprintf(stdout,
        "Usage: sng2sid <song.sng> <outfile> [options]\n"
        "Options:\n"
        "-Axx Set ADSR parameter for hardrestart in hex. DEFAULT=from song\n"
        "-Bx  enable/disable buffered SID writes. DEFAULT=disabled\n"
        "-Cx  enable/disable zeropage ghost registers. DEFAULT=disabled\n"
        "-Dx  enable/disable sound effect support. DEFAULT=disabled\n"
        "-Ex  enable/disable volume change support. DEFAULT=disabled\n"
        "-Hx  enable/disable storing of author info. DEFAULT=disabled\n"
        "-Ix  enable/disable optimizations. DEFAULT=enabled\n"
        "-Jx  enable/disable full buffering. DEFAULT=disabled\n"
        "-Lxx SID memory location in hex. DEFAULT=D400\n"
        "-N   Use NTSC timing\n"
        "-Oxx Set pulseoptimization/skipping (0 = off, 1 = on) DEFAULT=on\n"
        "-P   Use PAL timing (DEFAULT)\n"
        "-Rxx Set realtime-effect optimization/skipping (0 = off, 1 = on) DEFAULT=on\n"
        "-Sxx Set speed multiplier (0 for 25Hz, 1 for 1x, 2 for 2x etc.) DEFAULT=from song\n"
        "-Wxx player memory location highbyte in hex. DEFAULT=1000\n"
        "-Zxx zeropage memory location in hex. DEFAULT=FC\n"
        "-?   Show options\n");
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
    if (argc < 3 || (argv[1][0] == '-' && argv[1][1] == '?')) {
        usage();
        return argc < 3 ? 1 : 0;
    }

    gt::ExportOptions opt;
    opt.format = format_from_path(argv[2]);

    for (int c = 3; c < argc; c++) {
        if (argv[c][0] != '-') {
            fprintf(stderr, "error: unknown option\n");
            usage();
            return 1;
        }
        switch (toupper(unsigned(argv[c][1]))) {
        case '?':
            usage();
            return 0;
        case 'A': {
            unsigned v = 0;
            sscanf(&argv[c][2], "%x", &v);
            opt.adparam_override = int(v);
            break;
        }
        case 'L': {
            unsigned v = 0;
            sscanf(&argv[c][2], "%x", &v);
            opt.sid_addr = uint16_t(v);
            break;
        }
        case 'O': {
            unsigned v = 1;
            sscanf(&argv[c][2], "%u", &v);
            opt.optimize_pulse = v != 0;
            break;
        }
        case 'R': {
            unsigned v = 1;
            sscanf(&argv[c][2], "%u", &v);
            opt.optimize_realtime = v != 0;
            break;
        }
        case 'S': sscanf(&argv[c][2], "%d", &opt.multiplier_override); break;
        case 'N': opt.ntsc = true; break;
        case 'P': opt.ntsc = false; break;
        case 'B': opt.buffered = argv[c][2] == '1'; break;
        case 'D': opt.sound_effects = argv[c][2] == '1'; break;
        case 'E': opt.volume = argv[c][2] == '1'; break;
        case 'H': opt.author_info = argv[c][2] == '1'; break;
        case 'C': opt.zp_ghostregs = argv[c][2] == '1'; break;
        case 'I': opt.optimize = argv[c][2] != '0'; break;
        case 'J': opt.full_buffered = argv[c][2] == '1'; break;
        case 'W': {
            unsigned v = 0x10;
            sscanf(&argv[c][2], "%x", &v);
            opt.player_addr = uint16_t(v << 8);
            break;
        }
        case 'Z': {
            unsigned v = 0xfc;
            sscanf(&argv[c][2], "%x", &v);
            opt.zp_base = uint8_t(v);
            break;
        }
        default:
            fprintf(stderr, "error: unknown option\n");
            usage();
            return 1;
        }
    }

    if (opt.multiplier_override > 16) opt.multiplier_override = 16;
    opt.player_addr &= 0xff00;
    opt.sid_addr &= 0xffff;

    try {
        gt::Song song;
        song.load(argv[1]);
        auto bytes = gt::export_song(song, opt);
        std::ofstream out(argv[2], std::ios::binary);
        if (!out) {
            fprintf(stderr, "error: could not open output file '%s'.\n", argv[2]);
            return 1;
        }
        out.write(reinterpret_cast<char const*>(bytes.data()), std::streamsize(bytes.size()));
        fprintf(stdout, "sng2sid: wrote %s (%zu bytes)\n", argv[2], bytes.size());
        return 0;
    } catch (std::exception const& e) {
        fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
}
