#include <cstdio>
#include <cstring>
#include "gtsong.hpp"

namespace gt {
namespace {

uint8_t fread8(FILE *file) {
    uint8_t b;
    fread(&b, 1, 1, file);
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
    FILE* file = fopen(filename, "rb");
    if (!file) return false;
    char ident[4];
    fread(ident, 4, 1, file);
    if (memcmp(ident, "GTS5", 4)) {
        fclose(file);
        return false;
    }

    clear();

    // read infotexts
    fread(songname, 1, sizeof(songname), file);
    fread(authorname, 1, sizeof(authorname), file);
    fread(copyrightname, 1, sizeof(copyrightname), file);

    // read songorderlists
    int amount = fread8(file);
    for (int d = 0; d < amount; d++) {
        for (int c = 0; c < MAX_CHN; c++) {
            int loadsize = fread8(file) + 1;
            fread(songorder[d][c], loadsize, 1, file);
        }
    }
    // read instruments
    amount = fread8(file);
    for (int c = 1; c <= amount; c++) {
        instr[c].ad        = fread8(file);
        instr[c].sr        = fread8(file);
        instr[c].ptr[WTBL] = fread8(file);
        instr[c].ptr[PTBL] = fread8(file);
        instr[c].ptr[FTBL] = fread8(file);
        instr[c].ptr[STBL] = fread8(file);
        instr[c].vibdelay  = fread8(file);
        instr[c].gatetimer = fread8(file);
        instr[c].firstwave = fread8(file);
        fread(&instr[c].name, MAX_INSTRNAMELEN, 1, file);
    }
    // read tables
    for (int c = 0; c < MAX_TABLES; c++) {
        int loadsize = fread8(file);
        fread(ltable[c], loadsize, 1, file);
        fread(rtable[c], loadsize, 1, file);
    }
    // read patterns
    amount = fread8(file);
    for (int c = 0; c < amount; c++) {
        int length = fread8(file) * 4;
        fread(pattern[c], length, 1, file);
    }
    fclose(file);

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
