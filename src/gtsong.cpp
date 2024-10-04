#include <cstdio>
#include <cstring>
#include <fstream>
#include "gtsong.hpp"

namespace gt {
namespace {

template <class T>
bool read(std::istream& stream, T& v) {
    stream.read((char*) &v, sizeof(T));
    return stream.good();
}

uint8_t read8(std::istream& stream) {
    uint8_t b;
    read(stream, b);
    return b;
}

template <class T>
bool write(std::ostream& stream, T const& v) {
    stream.write((char const*) &v, sizeof(T));
    return stream.good();
}

} // namespace


void Song::count_pattern_lengths() {
    highestusedpattern = 0;
    highestusedinstr   = 0;
    for (int c = 0; c < MAX_PATT; c++) {
        int d;
        for (d = 0; d <= MAX_PATTROWS; d++) {
            if (pattern[c][d * 4] == ENDPATT) break;
            if (pattern[c][d * 4] != REST ||
                pattern[c][d * 4 + 1] ||
                pattern[c][d * 4 + 2] ||
                pattern[c][d * 4 + 3])
            {
                highestusedpattern = c;
            }
            highestusedinstr = std::max(highestusedinstr, int(pattern[c][d * 4 + 1]));
        }
        pattlen[c] = d;
    }

    for (int c = 0; c < MAX_CHN; c++) {
        int d;
        for (d = 0; d < MAX_SONGLEN; d++) {
            if (songorder[c][d] >= LOOPSONG) break;
            if (songorder[c][d] < REPEAT)
            {
                highestusedpattern = std::max(highestusedpattern, int(songorder[c][d]));
            }
        }
        songlen[c] = d;
    }
}

void Song::clear_pattern(int p) {
    pattern[p] = {};
    for (int c = 0; c < 32; c++) pattern[p][c * 4] = REST;
    for (int c = 32; c <= MAX_PATTROWS; c++) pattern[p][c * 4] = ENDPATT;
}

void Song::clear_instr(int num) {
    instr[num] = {};
    if (num) {
        //if (multiplier) instr[num].gatetimer = 2 * multiplier;
        //else instr[num].gatetimer = 1;
        instr[num].gatetimer = 2;
        instr[num].firstwave = 0x9;
    }
}

void Song::clear() {
    songorder = {};
    for (int c = 0; c < MAX_CHN; c++) {
        songorder[c][0] = c;
        songorder[c][1] = LOOPSONG;
    }
    songname = {};
    authorname = {};
    copyrightname = {};
    for (int c = 0; c < MAX_PATT; c++) clear_pattern(c);
    for (int c = 0; c < MAX_INSTR; c++) clear_instr(c);
    ltable = {};
    rtable = {};
    count_pattern_lengths();
}


bool Song::load(char const* filename) {
    std::ifstream stream(filename, std::ios::binary);
    if (!stream.is_open()) return false;
    return load(stream);
}
bool Song::load(uint8_t const* data, size_t size) {
    struct MemBuf : std::streambuf {
        MemBuf(uint8_t const* data, size_t size) {
            char* p = (char*) data;
            this->setg(p, p, p + size);
        }
    } membuf(data, size);
    std::istream stream(&membuf);
    return load(stream);
}
bool Song::load(std::istream& stream) {
    char ident[4];
    stream.read(ident, 4);
    if (memcmp(ident, "GTS5", 4) != 0) return false;

    clear();

    // read infotexts
    read(stream, songname);
    read(stream, authorname);
    read(stream, copyrightname);

    // read songorderlists
    int amount = read8(stream);
    printf("%d\n", amount);
    for (int c = 0; c < MAX_CHN; c++) {
        int loadsize = read8(stream) + 1;
        stream.read((char*) songorder[c].data(), loadsize);
    }
    for (int d = 1; d < amount; d++) {
        for (int c = 0; c < MAX_CHN; c++) {
            int loadsize = read8(stream) + 1;
            stream.ignore(loadsize);
        }
    }

    // read instruments
    amount = read8(stream);
    for (int c = 1; c <= amount; c++) {
        instr[c].ad        = read8(stream);
        instr[c].sr        = read8(stream);
        instr[c].ptr[WTBL] = read8(stream);
        instr[c].ptr[PTBL] = read8(stream);
        instr[c].ptr[FTBL] = read8(stream);
        instr[c].ptr[STBL] = read8(stream);
        instr[c].vibdelay  = read8(stream);
        instr[c].gatetimer = read8(stream);
        instr[c].firstwave = read8(stream);
        read(stream, instr[c].name);
    }
    // read tables
    for (int c = 0; c < MAX_TABLES; c++) {
        int loadsize = read8(stream);
        stream.read((char*) ltable[c].data(), loadsize);
        stream.read((char*) rtable[c].data(), loadsize);
    }
    // read patterns
    amount = read8(stream);
    for (int c = 0; c < amount; c++) {
        int length = read8(stream) * 4;
        stream.read((char*) pattern[c].data(), length);
    }

    count_pattern_lengths();

    return true;
}


bool Song::save(char const* filename) {
    std::ofstream stream(filename, std::ios::binary);
    if (!stream.is_open()) return false;
    return save(stream);
}
bool Song::save(std::ostream& stream) {
    count_pattern_lengths();

    stream.write("GTS5", 4);

    for (int c = 1; c < MAX_INSTR; c++) {
        if (instr[c].ad || instr[c].sr || instr[c].ptr[0] || instr[c].ptr[1] ||
            instr[c].ptr[2] || instr[c].ptr[3] || instr[c].vibdelay)
        {
            highestusedinstr = std::max(highestusedinstr, c);
        }
    }

    // infotexts
    write(stream, songname);
    write(stream, authorname);
    write(stream, copyrightname);

    // songorderlists
    int amount = 1;
    write<uint8_t>(stream, amount);
    for (int c = 0; c < MAX_CHN; c++) {
        int length = songlen[c] + 1;
        write<uint8_t>(stream, length);
        int writebytes = length + 1;
        stream.write((char const*) songorder[c].data(), writebytes);
    }

    // instruments
    write<uint8_t>(stream, highestusedinstr);
    for (int c = 1; c <= highestusedinstr; c++) {
        write(stream, instr[c].ad);
        write(stream, instr[c].sr);
        write(stream, instr[c].ptr[WTBL]);
        write(stream, instr[c].ptr[PTBL]);
        write(stream, instr[c].ptr[FTBL]);
        write(stream, instr[c].ptr[STBL]);
        write(stream, instr[c].vibdelay);
        write(stream, instr[c].gatetimer);
        write(stream, instr[c].firstwave);
        write(stream, instr[c].name);
    }
    // Write tables
    for (int c = 0; c < MAX_TABLES; c++) {
        int writebytes = gettablelen(c);
        write<uint8_t>(stream, writebytes);
        stream.write((char const*) ltable[c].data(), writebytes);
        stream.write((char const*) rtable[c].data(), writebytes);
    }
    // Write patterns
    amount = highestusedpattern + 1;
    write<uint8_t>(stream, amount);
    for (int c = 0; c < amount; c++) {
        int length = pattlen[c] + 1;
        write<uint8_t>(stream, length);
        stream.write((char const*) pattern[c].data(), length * 4);
    }

    return true;
}

} // namespace gt
