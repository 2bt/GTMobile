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


void fwrite8(FILE *file, uint8_t b) {
    fwrite(&b, 1, 1, file);
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
            if (pattern[c][d * 4 + 1] > highestusedinstr) highestusedinstr = pattern[c][d * 4 + 1];
        }
        pattlen[c] = d;
    }

    for (int e = 0; e < MAX_SONGS; e++) {
        for (int c = 0; c < MAX_CHN; c++) {
            int d;
            for (d = 0; d < MAX_SONGLEN; d++) {
                if (songorder[e][c][d] >= LOOPSONG) break;
                if (songorder[e][c][d] < REPEAT &&
                    songorder[e][c][d] > highestusedpattern)
                {
                    highestusedpattern = songorder[e][c][d];
                }
            }
            songlen[e][c] = d;
        }
    }
}

void Song::clear_pattern(int p) {
    memset(pattern[p], 0, MAX_PATTROWS * 4);
    for (int c = 0; c < 32; c++) pattern[p][c * 4] = REST;
    for (int c = 32; c <= MAX_PATTROWS; c++) pattern[p][c * 4] = ENDPATT;
}

void Song::clear_instr(int num) {
    memset(&instr[num], 0, sizeof(Instr));
    if (num) {
        //if (multiplier) instr[num].gatetimer = 2 * multiplier;
        //else instr[num].gatetimer = 1;
        instr[num].gatetimer = 2;
        instr[num].firstwave = 0x9;
    }
}

void Song::clear() {
    for (int c = 0; c < MAX_CHN; c++) {
        for (int d = 0; d < MAX_SONGS; d++) {
            memset(&songorder[d][c][0], 0, MAX_SONGLEN + 2);
            if (!d) {
                songorder[d][c][0] = c;
                songorder[d][c][1] = LOOPSONG;
            }
            else {
                songorder[d][c][0] = LOOPSONG;
            }
        }
    }
    memset(songname, 0, sizeof songname);
    memset(authorname, 0, sizeof authorname);
    memset(copyrightname, 0, sizeof copyrightname);
    for (int c = 0; c < MAX_PATT; c++) clear_pattern(c);
    for (int c = 0; c < MAX_INSTR; c++) clear_instr(c);
    for (int c = MAX_TABLES - 1; c >= 0; c--) {
        memset(ltable[c], 0, MAX_TABLELEN);
        memset(rtable[c], 0, MAX_TABLELEN);
    }
    count_pattern_lengths();
}


bool Song::load(char const* filename) {
    std::ifstream stream(filename, std::ios::binary);
    if (!stream.is_open()) return false;
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
    for (int d = 0; d < amount; d++) {
        for (int c = 0; c < MAX_CHN; c++) {
            int loadsize = read8(stream) + 1;
            stream.read((char*) songorder[d][c], loadsize);
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
        stream.read((char*) ltable[c], loadsize);
        stream.read((char*) rtable[c], loadsize);
    }
    // read patterns
    amount = read8(stream);
    for (int c = 0; c < amount; c++) {
        int length = read8(stream) * 4;
        stream.read((char*) pattern[c], length);
    }

    count_pattern_lengths();

    return true;
}


bool Song::save(char const* filename) {
    count_pattern_lengths();

    FILE* file = fopen(filename, "wb");
    if (!file) return false;
    fwrite("GTS5", 1, 4, file);

    //for (int c = 1; c < MAX_INSTR; c++) {
    //    if (instr[c].ad || instr[c].sr || instr[c].ptr[0] || instr[c].ptr[1] ||
    //        instr[c].ptr[2] || instr[c].vibdelay || instr[c].ptr[3])
    //    {
    //        if (c > highestusedinstr) highestusedinstr = c;
    //    }
    //}

    // infotexts
    fwrite(songname, 1, sizeof(songname), file);
    fwrite(authorname, 1, sizeof(authorname), file);
    fwrite(copyrightname, 1, sizeof(copyrightname), file);

    // songorderlists
    int c = MAX_SONGS - 1;
    for (;;) {
        if (songlen[c][0] && songlen[c][1] && songlen[c][2]) break;
        if (c == 0) break;
        --c;
    }
    int amount = c + 1;
    fwrite8(file, amount);
    for (int d = 0; d < amount; d++) {
        for (int c = 0; c < MAX_CHN; c++) {
            int length = songlen[d][c] + 1;
            fwrite8(file, length);
            int writebytes = length + 1;
            fwrite(songorder[d][c], 1, writebytes, file);
        }
    }
    // instruments
    fwrite8(file, highestusedinstr);
    for (int c = 1; c <= highestusedinstr; c++) {
        fwrite8(file, instr[c].ad);
        fwrite8(file, instr[c].sr);
        fwrite8(file, instr[c].ptr[WTBL]);
        fwrite8(file, instr[c].ptr[PTBL]);
        fwrite8(file, instr[c].ptr[FTBL]);
        fwrite8(file, instr[c].ptr[STBL]);
        fwrite8(file, instr[c].vibdelay);
        fwrite8(file, instr[c].gatetimer);
        fwrite8(file, instr[c].firstwave);
        fwrite(&instr[c].name, 1, MAX_INSTRNAMELEN, file);
    }
    // Write tables
    for (int c = 0; c < MAX_TABLES; c++) {
        int writebytes = gettablelen(c);
        fwrite8(file, writebytes);
        fwrite(ltable[c], 1, writebytes, file);
        fwrite(rtable[c], 1, writebytes, file);
    }
    // Write patterns
    amount = highestusedpattern + 1;
    fwrite8(file, amount);
    for (int c = 0; c < amount; c++) {
        int length = pattlen[c] + 1;
        fwrite8(file, length);
        fwrite(pattern[c], 1, length * 4, file);
    }
    fclose(file);

    return true;
}

} // namespace gt
