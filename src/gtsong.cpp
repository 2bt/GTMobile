#include <cstdio>
#include <cstring>
#include <fstream>
#include <cassert>
#include "gtsong.hpp"
#include "log.hpp"

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


void Song::clear() {
    *this = {};
    song_order[1][0].pattnum = 1;
    song_order[2][0].pattnum = 2;
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
    std::array<char, 4> ident;
    read(stream, ident);
    if (ident != std::array<char, 4>{'G', 'T', 'S', '5'}) return false;

    clear();
    read(stream, song_name);
    read(stream, author_name);
    read(stream, copyright_name);

    // read songorderlists
    int amount = read8(stream);
    if (amount != 1) {
        LOGE("Song::load: multiple songs are not supported");
        clear();
        return false;
    }

    for (int c = 0; c < MAX_CHN; c++) {
        int buffer_len = read8(stream) + 1;
        assert(buffer_len >= 3);
        std::array<uint8_t, 256> buffer;
        stream.read((char*) buffer.data(), buffer_len);

        assert(buffer[buffer_len - 2] == LOOPSONG);
        int loop  = buffer[buffer_len - 1];
        int pos   = 0;
        int trans = 0;
        auto& order = song_order[c];
        for (int i = 0; i < buffer_len;) {
            uint8_t x = buffer[i++];
            if (x == LOOPSONG) break;
            if (pos >= int(order.size())) {
                LOGE("Song::load: max song length exceeded");
                clear();
                return false;
            }
            // transpose
            if (x >= TRANSDOWN && x < LOOPSONG) {
                if (i <= buffer[buffer_len - 1]) --loop;
                trans = x - TRANSUP;
                x = buffer[i++];
            }
            if (x >= REPEAT && x < TRANSDOWN) {
                LOGE("Song::load: repeat command not supported");
                clear();
                return false;
            }
            if (x >= MAX_PATT) {
                LOGE("Song::load: invalid pattern number");
                clear();
                return false;
            }
            order[pos].trans   = trans;
            order[pos].pattnum = x;
            ++pos;
        }
        if (c == 0) {
            song_len  = pos;
            song_loop = loop;
        }
        else {
            if (pos != song_len) {
                LOGE("Song::load: song length mismatch");
                clear();
                return false;
            }
            if (loop != song_loop) {
                LOGE("Song::load: song loop mismatch");
                clear();
                return false;
            }
        }
    }

    // read instruments
    amount = read8(stream);
    for (int c = 1; c <= amount; c++) {
        Instrument& instr = instruments[c];
        instr.ad        = read8(stream);
        instr.sr        = read8(stream);
        read(stream, instr.ptr);
        instr.vibdelay  = read8(stream);
        instr.gatetimer = read8(stream);
        instr.firstwave = read8(stream);
        read(stream, instr.name);
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
        int len = read8(stream);
        std::array<uint8_t, 1024> buffer;
        stream.read((char*) buffer.data(), len * 4);
        Pattern& patt = patterns[c];
        patt.len = len - 1;
        assert(patt.len > 0);
        assert(patt.len <= int(patt.rows.size()));
        int instr = 0;
        for (int i = 0; i < patt.len; ++i) {
            PatternRow row = {
                buffer[i * 4 + 0],
                buffer[i * 4 + 1],
                buffer[i * 4 + 2],
                buffer[i * 4 + 3],
            };
            // add missing instr to notes
            if (row.instr > 0) instr = row.instr;
            else if (instr > 0 && row.note <= LASTNOTE) row.instr = instr;
            patt.rows[i] = row;
        }
    }

    return true;
}


bool Song::save(char const* filename) {
    std::ofstream stream(filename, std::ios::binary);
    if (!stream.is_open()) return false;
    return save(stream);
}


bool Song::save(std::ostream& stream) {

    assert(song_len <= MAX_SONGLEN / 2);

    stream.write("GTS5", 4);
    write(stream, song_name);
    write(stream, author_name);
    write(stream, copyright_name);

    // songorderlists
    write<uint8_t>(stream, 1);
    for (auto const& order : song_order) {
        // TODO: remove transpose
        write<uint8_t>(stream, song_len * 2 + 1);
        for (int r = 0; r < song_len; ++r) {
            write<uint8_t>(stream, order[r].trans + TRANSUP);
            write<uint8_t>(stream, order[r].pattnum);
        }
        write<uint8_t>(stream, LOOPSONG);
        write<uint8_t>(stream, song_loop);
    }

    // instruments
    int max_used_instr = 1;
    constexpr Instrument emtpy_instr = {};
    for (int i = 0; i < MAX_INSTR; i++) {
        if (memcmp(&instruments[i], &emtpy_instr, sizeof(Instrument)) != 0) {
            max_used_instr = i;
        }
    }
    write<uint8_t>(stream, max_used_instr);
    for (int i = 1; i <= max_used_instr; i++) {
        Instrument& instr = instruments[i];
        write(stream, instr.ad);
        write(stream, instr.sr);
        write(stream, instr.ptr);
        write(stream, instr.vibdelay);
        write(stream, instr.gatetimer);
        write(stream, instr.firstwave);
        write(stream, instr.name);
    }

    // tables
    for (int i = 0; i < MAX_TABLES; i++) {
        int c = MAX_TABLELEN - 1;
        for (; c >= 0; c--) {
            if (ltable[i][c] | rtable[i][c]) break;
        }
        ++c;
        write<uint8_t>(stream, c);
        stream.write((char const*) ltable[i].data(), c);
        stream.write((char const*) rtable[i].data(), c);
    }

    // patterns
    int max_used_patt = 0;
    for (int i = 0; i < MAX_PATT; ++i) {
        Pattern const& patt = patterns[i];
        for (int r = 0; r < patt.len; ++r) {
            auto row = patt.rows[r];
            if (row.note != gt::REST || row.instr || row.command) {
                max_used_patt = i;
                break;
            }
        }
    }
    write<uint8_t>(stream, max_used_patt + 1);
    for (int i = 0; i <= max_used_patt; i++) {
        Pattern const& patt = patterns[i];
        write<uint8_t>(stream, patt.len + 1);
        for (int r = 0; r < patt.len; ++r) {
            write(stream, patt.rows[r]);
        }
        write<uint8_t>(stream, ENDPATT);
        write<uint8_t>(stream, 0);
        write<uint8_t>(stream, 0);
        write<uint8_t>(stream, 0);
    }

    return true;
}

} // namespace gt
