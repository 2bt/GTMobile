#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>
#include <cassert>
#include <algorithm>
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

void load_error(std::string msg) {
    LOGE("Song::load: %s", msg.c_str());
    throw LoadError(std::move(msg));
}

} // namespace


void Song::clear() {
    *this = {};
    song_order[1][0].pattnum = 1;
    song_order[2][0].pattnum = 2;
}

void Song::load(char const* filename) {
    std::ifstream stream(filename, std::ios::binary);
    if (!stream.is_open()) load_error("cannot open file");
    load(stream);
}
void Song::load(uint8_t const* data, size_t size) {
    struct MemBuf : std::streambuf {
        MemBuf(uint8_t const* data, size_t size) {
            char* p = (char*) data;
            this->setg(p, p, p + size);
        }
    } membuf(data, size);
    std::istream stream(&membuf);
    load(stream);
}
void Song::load(std::istream& stream) {
    std::array<char, 4> ident;
    read(stream, ident);
    if (ident != std::array<char, 4>{'G', 'T', 'S', '5'}) {
        load_error("bad file format");
    }

    clear();
    read(stream, song_name);
    read(stream, author_name);
    read(stream, copyright_name);

    // read songorderlists
    int amount = read8(stream);
    if (amount != 1) load_error("multiple songs not supported");
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
                load_error("max song length exceeded");
            }
            // transpose
            if (x >= TRANSDOWN && x < LOOPSONG) {
                if (i <= buffer[buffer_len - 1]) --loop;
                trans = x - TRANSUP;
                x = buffer[i++];
            }
            if (x >= REPEAT && x < TRANSDOWN) {
                load_error("repeat command not supported");
            }
            if (x >= MAX_PATT) {
                load_error("invalid pattern number");
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
            if (pos != song_len) load_error("song length mismatch");
            if (loop != song_loop) load_error("song loop mismatch");
        }
    }

    // read instruments
    int instr_count = read8(stream);
    for (int c = 1; c <= instr_count; c++) {
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
        int len = read8(stream);
        stream.read((char*) ltable[c].data(), len);
        stream.read((char*) rtable[c].data(), len);
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

    // set up speed table
    // 01 - 1f: portamento
    // 21 - 3f: vibrato
    // 41 - 5f: funk tempo
    std::array<uint8_t, MAX_TABLELEN> porta = {};
    std::array<uint8_t, MAX_TABLELEN> vib   = {};
    std::array<uint8_t, MAX_TABLELEN> funk  = {};
    for (Instrument& instr : instruments) {
        if (instr.ptr[STBL] > 0) vib[instr.ptr[STBL]] = 1;
    }
    for (Pattern& patt : patterns) {
        for (PatternRow& row : patt.rows) {
            if (row.data == 0) continue;
            if (row.command >= CMD_PORTAUP && row.command <= CMD_TONEPORTA) porta[row.data - 1] = 1;
            if (row.command == CMD_VIBRATO)                                 vib[row.data - 1] = 1;
            if (row.command == CMD_FUNKTEMPO)                               funk[row.data - 1] = 1;
        }
    }
    for (int i = 0; i < MAX_TABLELEN; ++i) {
        if ((ltable[WTBL][i] & 0xf0) != 0xf0) continue;
        uint8_t data = rtable[WTBL][i];
        if (data == 0) continue;
        uint8_t cmd = ltable[WTBL][i] & 0xf;
        if (cmd >= CMD_PORTAUP && cmd <= CMD_TONEPORTA) porta[data - 1] = 1;
        if (cmd == CMD_VIBRATO)                         vib[data - 1] = 1;
        if (cmd == CMD_FUNKTEMPO)                       funk[data - 1] = 1;
    }
    std::array<uint8_t, MAX_TABLELEN> lspeed = ltable[STBL];
    std::array<uint8_t, MAX_TABLELEN> rspeed = rtable[STBL];
    ltable[STBL] = {};
    rtable[STBL] = {};
    size_t porta_pos = 0x00;
    size_t vib_pos   = 0x20;
    size_t funk_pos  = 0x40;
    for (size_t i = 0; i < lspeed.size(); ++i) {
        if (vib[i]) {
            ltable[STBL][vib_pos] = lspeed[i];
            rtable[STBL][vib_pos] = rspeed[i];
            vib[i] = vib_pos++;
        }
        if (porta[i]) {
            ltable[STBL][porta_pos] = lspeed[i];
            rtable[STBL][porta_pos] = rspeed[i];
            porta[i] = porta_pos++;
        }
        if (funk[i]) {
            ltable[STBL][funk_pos] = lspeed[i];
            rtable[STBL][funk_pos] = rspeed[i];
            funk[i] = funk_pos++;
        }
    }
    if (porta_pos >= 0x1f) load_error("too many portamenti");
    if (vib_pos >= 0x3f) load_error("too many vibratos");
    if (funk_pos >= 0x5f) load_error("too many funk tempi");
    for (Instrument& instr : instruments) {
        if (instr.ptr[STBL] > 0) instr.ptr[STBL] = vib[instr.ptr[STBL] - 1] + 1;
    }
    for (Pattern& patt : patterns) {
        for (PatternRow& row : patt.rows) {
            if (row.data == 0) continue;
            if (row.command >= CMD_PORTAUP && row.command <= CMD_TONEPORTA) row.data = porta[row.data - 1] + 1;
            if (row.command == CMD_VIBRATO)                                 row.data = vib[row.data - 1] + 1;
            if (row.command == CMD_FUNKTEMPO)                               row.data = funk[row.data - 1] + 1;
        }
    }
    for (int i = 0; i < MAX_TABLELEN; ++i) {
        if ((ltable[WTBL][i] & 0xf0) != 0xf0) continue;
        uint8_t& data = rtable[WTBL][i];
        if (data == 0) continue;
        uint8_t cmd = ltable[WTBL][i] & 0xf;
        if (cmd >= CMD_PORTAUP && cmd <= CMD_TONEPORTA) data = porta[data - 1] + 1;
        if (cmd == CMD_VIBRATO)                         data = vib[data - 1] + 1;
        if (cmd == CMD_FUNKTEMPO)                       data = funk[data - 1] + 1;
    }

    // TODO: check table jumps
    // we only allow for backward jumps to zero or within the table
    // we only allow for table addresses (in instruments and table ptr commands)


    // fix table ptr commands
    for (int cmd = CMD_SETWAVEPTR; cmd <= CMD_SETFILTERPTR; ++cmd) {
        constexpr char const* LABELS[] = { "WAVE", "PULS", "FILT" };
        int table = cmd - CMD_SETWAVEPTR;
        std::array<uint8_t, MAX_TABLELEN> addr_to_instr = {};

        // create initial mapping from table row to instrument
        for (int i = instr_count; i > 0; i--) {
            Instrument& instr = instruments[i];
            if (instr.ptr[table] == 0) continue;
            int addr = instr.ptr[table] - 1;
            addr_to_instr[addr] = i;
        }
        // apply mapping in patterns
        for (Pattern& patt : patterns) {
            for (PatternRow& row : patt.rows) {
                if (row.command != cmd || row.data == 0) continue;
                int addr = row.data - 1;
                if (addr_to_instr[addr] == 0) {
                    if (instr_count >= MAX_INSTR - 1) {
                        load_error("cannot remap table ptr command");
                    }
                    addr_to_instr[addr] = ++instr_count;
                    Instrument& instr = instruments[addr_to_instr[addr]];
                    sprintf(instr.name.data(), "%s PTR %02X", LABELS[table], addr + 1);
                    instr.ptr[table] = addr + 1;
                }
                row.data = addr_to_instr[addr];
            }
        }

        // apply mapping in wave table
        if (cmd == CMD_SETWAVEPTR) continue;
        for (int i = 0; i < MAX_TABLELEN; ++i) {
            if ((ltable[WTBL][i] & 0xf0) != 0xf0) continue;
            uint8_t& data = rtable[WTBL][i];
            if (data == 0 || (ltable[WTBL][i] & 0xf) != cmd) continue;
            int addr = data - 1;
            if (addr_to_instr[addr] == 0) {
                if (instr_count >= MAX_INSTR - 1) {
                    load_error("cannot remap table ptr command");
                }
                addr_to_instr[addr] = ++instr_count;
                Instrument& instr = instruments[addr_to_instr[addr]];
                char const* LABELS[] = { "", "PULS", "FILT" };
                sprintf(instr.name.data(), "%s PTR %02X X", LABELS[table], addr + 1);
                instr.ptr[table] = addr + 1;
            }
            data = addr_to_instr[addr];
        }
    }
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


    // fix table pointer commands
    // map instrument back to table row
    for (Pattern& patt : patterns) {
        for (PatternRow& row : patt.rows) {
            if (row.data == 0) continue;
            if (row.command >= CMD_SETWAVEPTR && row.command <= CMD_SETFILTERPTR) {
                int t = row.command - CMD_SETWAVEPTR;
                Instrument const& instr = instruments[row.data];
                row.data = instr.ptr[t];
            }
        }
    }
    for (int i = 0; i < MAX_TABLELEN; ++i) {
        if ((ltable[WTBL][i] & 0xf0) != 0xf0) continue;
        uint8_t& data = rtable[WTBL][i];
        if (data == 0) continue;
        uint8_t cmd = ltable[WTBL][i] & 0xf;
        if (cmd >= CMD_SETFILTERCTRL && cmd <= CMD_SETFILTERPTR) {
            int t = cmd - CMD_SETWAVEPTR;
            Instrument const& instr = instruments[data];
            data = instr.ptr[t];
        }
    }

    return true;
}

} // namespace gt
