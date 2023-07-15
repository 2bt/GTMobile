#include "app.hpp"
#include "gtsong.hpp"
#include "platform.hpp"
#include "gtplayer.hpp"
#include "sidengine.hpp"
#include "log.hpp"
#include "gfx.hpp"
#include <cmath>



namespace app {
namespace {

gt::Song   g_song;
gt::Player g_player(g_song);
SidEngine  g_sid_engine(MIXRATE);

gfx::Image  g_gui_img;
gfx::Image  g_mock_img;
gfx::Canvas g_canvas;
float       g_canvas_scale;
int16_t     g_canvas_offset;

int         W, H;

} // namespace


void audio_callback(int16_t* buffer, int length) {
    enum {
        SAMPLES_PER_FRAME = MIXRATE / 50,
    };
    static int sample = 0;
    while (length > 0) {
        if (sample == 0) {
            g_player.play_routine();
            for (int i = 0; i < 25; ++i) g_sid_engine.write(i, g_player.regs[i]);
        }
        int l = std::min(SAMPLES_PER_FRAME - sample, length);
        sample += l;
        if (sample == SAMPLES_PER_FRAME) sample = 0;
        length -= l;
        g_sid_engine.mix(buffer, l);
        buffer += l;
    }
}

void init() {
    LOGD("init");


    gfx::init();
    g_gui_img.init("gui.png");
    g_mock_img.init("mock00.png");



    // load song
    std::vector<uint8_t> buffer;
    platform::load_asset("Nordic_Scene_Review.sng", buffer);
    struct MemBuf : std::streambuf {
        MemBuf(uint8_t const* data, size_t size) {
            char* p = (char*)data;
            this->setg(p, p, p + size);
        }
    } membuf(buffer.data(), buffer.size());
    std::istream stream(&membuf);
    g_song.load(stream);
    g_player.init_song(0, gt::Player::PLAY_BEGINNING);


    platform::start_audio();
}

void free() {
    LOGD("free");
    gfx::free();

    g_canvas.free();
    g_gui_img.free();
    g_mock_img.free();
}

void resize(int width, int height) {
    gfx::set_screen_size({width, height});



    W = WIDTH;
    H = std::max<int16_t>(WIDTH * height / width, MIN_HEIGHT);
    // reinit canvas with POT dimensions
    {
        int w = 2;
        int h = 2;
        while (w < W) w *= 2;
        while (h < H) h *= 2;
        g_canvas.init({ w, h });
    }
    // set canvas offset and scale
    float scale_x = width  / float(W);
    float scale_y = height / float(H);
    if (scale_y < scale_x) {
        g_canvas_offset = (width - W * scale_y) * 0.5f;
        g_canvas_scale  = scale_y;
    }
    else {
        g_canvas_offset = 0;
        g_canvas_scale  = scale_x;
    }

}


void touch(int x, int y, bool pressed) {
    LOGD("touch %d %d %d", x, y, pressed);
//    gui::touch((x - g_canvas_offset) / g_canvas_scale, y / g_canvas_scale, pressed);
}

void key(int key, int unicode) {
    LOGD("key %d %d", key, unicode);
}




///////////////////////////////////////////////////////////////////////////////////////
namespace gui {
namespace color {
    constexpr u8vec4 make(uint32_t c, uint8_t a = 255) {
        return { uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c), a };
    }

    #define COLORS(X)\
        X(0x000000, black)\
        X(0x1D2B53, dark_blue)\
        X(0x7E2553, dark_purple)\
        X(0x008751, dark_green)\
        X(0xAB5236, brown)\
        X(0x5F574F, dark_grey)\
        X(0xC2C3C7, light_grey)\
        X(0xFFF1E8, white)\
        X(0xFF004D, red)\
        X(0xFFA300, orange)\
        X(0xFFEC27, yellow)\
        X(0x00E436, green)\
        X(0x29ADFF, blue)\
        X(0x83769C, lavender)\
        X(0xFF77A8, pink)\
        X(0xFFCCAA, light_peach)

    #define COLOR_NAME(_, n) n,
    #define COLOR_VALUE(v, _) color::make(v),

    enum { COLORS(COLOR_NAME) };
    constexpr u8vec4 colors[] = { COLORS(COLOR_VALUE) };
}




class DrawContext : public gfx::DrawContext {
public:

    int color_index = color::light_grey;


    void box(ivec2 pos, ivec2 size) {
        int style = 1;

        int s = 8;
        int o = 64;
        if (size.x < 16 || size.y < 16) {
            s = 4;
            o += 16;
        }
        if (size.x < 8 || size.y < 8) {
            s = 1;
            o += 16;
        }

        i16vec2 p0 = pos;
        i16vec2 p1 = pos + ivec2(s);
        i16vec2 p2 = pos + size - ivec2(s);
        i16vec2 p3 = pos + size;
        i16vec2 t0 = i16vec2(16 * style, o);
        i16vec2 t1 = t0 + ivec2(s);
        i16vec2 t2 = t1;
        i16vec2 t3 = t2 + ivec2(s);

        auto c = color::colors[color_index];

        auto i0  = add_vertex({ { p0.x, p0.y }, { t0.x, t0.y }, c });
        auto i1  = add_vertex({ { p1.x, p0.y }, { t1.x, t0.y }, c });
        auto i2  = add_vertex({ { p2.x, p0.y }, { t2.x, t0.y }, c });
        auto i3  = add_vertex({ { p3.x, p0.y }, { t3.x, t0.y }, c });
        auto i4  = add_vertex({ { p0.x, p1.y }, { t0.x, t1.y }, c });
        auto i5  = add_vertex({ { p1.x, p1.y }, { t1.x, t1.y }, c });
        auto i6  = add_vertex({ { p2.x, p1.y }, { t2.x, t1.y }, c });
        auto i7  = add_vertex({ { p3.x, p1.y }, { t3.x, t1.y }, c });
        auto i8  = add_vertex({ { p0.x, p2.y }, { t0.x, t2.y }, c });
        auto i9  = add_vertex({ { p1.x, p2.y }, { t1.x, t2.y }, c });
        auto i10 = add_vertex({ { p2.x, p2.y }, { t2.x, t2.y }, c });
        auto i11 = add_vertex({ { p3.x, p2.y }, { t3.x, t2.y }, c });
        auto i12 = add_vertex({ { p0.x, p3.y }, { t0.x, t3.y }, c });
        auto i13 = add_vertex({ { p1.x, p3.y }, { t1.x, t3.y }, c });
        auto i14 = add_vertex({ { p2.x, p3.y }, { t2.x, t3.y }, c });
        auto i15 = add_vertex({ { p3.x, p3.y }, { t3.x, t3.y }, c });

        m_indices.insert(m_indices.end(), {
            i0,  i1,  i5,  i0,  i5,  i4,
            i1,  i2,  i6,  i1,  i6,  i5,
            i2,  i3,  i7,  i2,  i7,  i6,
            i4,  i5,  i9,  i4,  i9,  i8,
            i5,  i6,  i10, i5,  i10, i9,
            i6,  i7,  i11, i6,  i11, i10,
            i8,  i9,  i13, i8,  i13, i12,
            i9,  i10, i14, i9,  i14, i13,
            i10, i11, i15, i10, i15, i14,
        });

        

    }



//    int font_offset = 16;
    int  font_offset = 208;
    ivec2 char_size = {8, 8};

    void character(ivec2 pos, uint8_t g) {
        ivec2 uv(g % 16 * char_size.x, (g - 32) / 16 * char_size.x + font_offset);
        rect(pos, char_size, uv, color::colors[color_index]);
    }
    void text(ivec2 pos, const char* text) {
        ivec2 p = pos;
        while (char c = *text++) {
            if (c == '\a') { // choose color
                color_index = *text++;
                continue;
            }

            if (c > 32 || c < 128) character(p, c);
            p.x += char_size.x;
        }
    }
};

} // namespace gui






void draw_song() {

    gui::DrawContext dc;

    dc.rect({}, g_mock_img.size(), {});
    gfx::draw(dc, g_mock_img);   
    dc.clear();



    enum {
        SONG_NR = 0,

        MAX_SONG_ROWS_ON_SCREEN = 12,
        MAX_PATTERN_ROWS_ON_SCREEN = 32,
    };

    int selected_chan = 0;

    uint8_t  songptr;
    uint8_t  pattnum;
    uint32_t pattptr;
    g_player.get_chan_info(selected_chan, songptr, pattnum, pattptr);

    char line[16];
    ivec2 cursor = {0, 32};

    // song table
    {
        int len = g_song.songlen[SONG_NR][selected_chan];
        int current_row = std::max<int>(0, songptr - 1);

        int start_row = clamp(len - MAX_SONG_ROWS_ON_SCREEN, 0, current_row - MAX_SONG_ROWS_ON_SCREEN / 2);

        for (int i = 0; i < MAX_SONG_ROWS_ON_SCREEN; ++i) {
            int row = start_row + i;
            if (row >= len) {
                cursor.x = 0;
                cursor.y += 10;
                continue;
            }

            // highlight background
            if (row == current_row) {
                dc.color_index = gui::color::dark_blue;
                dc.box(cursor, {8 * 37, 10});
            }
            dc.color_index = gui::color::light_grey;

            cursor.x += 8;
            sprintf(line, "%02X", row);
            dc.text(cursor, line);
            cursor.x += 32;

            for (int c = 0; c < 3; ++c) {
                uint8_t v = g_song.songorder[SONG_NR][c][row];

                if (v == gt::LOOPSONG) {
                    sprintf(line, "RESTART    ");
                }
                else if (v >= gt::TRANSUP) {
                    sprintf(line, "TRANSP +%X ", v & 0xf);
                }
                else if (v >= gt::TRANSDOWN) {
                    sprintf(line, "TRANSP -%X ", v & 0xf);
                }
                else if (v >= gt::REPEAT) {
                    sprintf(line, "REPEAT %X  ", v & 0xf);
                }
                else {
                    sprintf(line, "%02X        ", v);
                }

                dc.text(cursor, line);
                cursor.x += 11 * 8;
            }
            cursor.x = 0;
            cursor.y += 10;

        }
    }

    cursor.x = 0;
    cursor.y += 10;


    int pattern_len = g_song.pattlen[pattnum];
    int current_row = pattptr / 4;
    if (current_row > pattern_len) current_row = pattern_len;



    // patterns
    int start_row = clamp(pattern_len - MAX_PATTERN_ROWS_ON_SCREEN, 0, current_row - MAX_PATTERN_ROWS_ON_SCREEN / 2);

    for (int i = 0; i < MAX_PATTERN_ROWS_ON_SCREEN; ++i) {
        int row = start_row + i;
        if (row >= pattern_len) break;

        // highlight background
        if (row == current_row) {
            dc.color_index = gui::color::dark_blue;
            dc.box(cursor, {8 * 37, 10});
        }
        dc.color_index = gui::color::light_grey;

        cursor.x += 8;
        sprintf(line, "%02X", row);
        dc.text(cursor, line);
        cursor.x += 32;

        for (int c = 0; c < 3; ++c) {

            uint8_t  songptr;
            uint8_t  pattnum;
            uint32_t pattptr;
            g_player.get_chan_info(c, songptr, pattnum, pattptr);
            uint8_t const* p = g_song.pattern[pattnum] + row * 4;

            if (*p == 0xff) break;

            int note  = p[0];
            int instr = p[1];
            int cmd   = p[2];
            int arg   = p[3];

            if (note == gt::REST) {
                dc.color_index = gui::color::dark_grey;
                dc.text(cursor, "...");
            }
            else if (note == gt::KEYOFF) {
                dc.color_index = gui::color::light_grey;
                dc.text(cursor, "===");
            }
            else if (note == gt::KEYOFF) {
                dc.color_index = gui::color::light_grey;
                dc.text(cursor, "===");
            }
            else {
                dc.color_index = gui::color::light_grey;
                sprintf(line, "%c%c%d", "CCDDEFFGGAAB"[note % 12],
                              "-#-#--#-#-#-"[note % 12],
                              (note - gt::FIRSTNOTE) / 12);
                dc.text(cursor, line);
            }
            cursor.x += 28;

            if (instr == 0) {
                dc.color_index = gui::color::dark_grey;
                dc.text(cursor, "..");
            }
            else {
                dc.color_index = gui::color::light_grey;
                sprintf(line, "%02X", instr);
                dc.text(cursor, line);
            }
            cursor.x += 20;

            if (cmd == 0) {
                dc.color_index = gui::color::dark_grey;
                dc.text(cursor, "...");
            }
            else {
                dc.color_index = gui::color::light_grey;
                sprintf(line, "%X%02X", cmd, arg);
                dc.text(cursor, line);
            }
            cursor.x += 40;
        }
        cursor.x = 0;
        cursor.y += 10;
    }


    gfx::draw(dc, g_gui_img);
}




void draw() {
    gfx::set_canvas(g_canvas);
    gfx::set_blend(true);
    gfx::clear(0.1f, 0.3f, 0.2f);


    draw_song();
//    g_canvas_scale = 1;

    // draw canvas
    gfx::set_canvas();
    gfx::set_blend(false);
    gfx::clear(0, 0, 0);
    u8vec4 white(255);
    ivec2 p(g_canvas_offset, 0);
    ivec2 s(W * g_canvas_scale, H * g_canvas_scale);
    gfx::DrawContext dc;
    dc.add_vertex({p, ivec2{0, H}, white});
    dc.add_vertex({ivec2(p.x + s.x, 0), ivec2{W, H}, white});
    dc.add_vertex({p + s, ivec2{W, 0}, white});
    dc.add_vertex({ivec2{p.x, s.y}, {0, 0}, white});
    dc.add_index(0);
    dc.add_index(1);
    dc.add_index(2);
    dc.add_index(0);
    dc.add_index(2);
    dc.add_index(3);
    gfx::draw(dc, g_canvas);
}


} // namespace app
