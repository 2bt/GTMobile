#include "app.hpp"
#include "gtsong.hpp"
#include "platform.hpp"
#include "gtplayer.hpp"
#include "sidengine.hpp"
#include "log.hpp"
#include "gui.hpp"
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


//    platform::start_audio();
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

    // reinit canvas with POT dimensions
    int w = 2;
    int h = 2;
    while (w < W) w *= 2;
    while (h < H) h *= 2;
    g_canvas.init({ w, h });
    if (g_canvas_scale != (int) g_canvas_scale) {
        g_canvas.set_filter(gfx::Texture::LINEAR);
    }
}


void touch(int x, int y, bool pressed) {
    gui::touch((x - g_canvas_offset) / g_canvas_scale, y / g_canvas_scale, pressed);
}

void key(int key, int unicode) {
    LOGD("key %d %d", key, unicode);
}




void draw_song() {

    gui::DrawContext dc;

    dc.rect({}, g_mock_img.size(), {});
    gfx::draw(dc, g_mock_img);
    dc.clear();



    enum {
        SONG_NR = 0,

        MAX_SONG_ROWS_ON_SCREEN = 8,
        MAX_PATTERN_ROWS_ON_SCREEN = 28,
    };

    enum {
        ROW_HEIGHT = 11,
    };

    int selected_chan = 0;

    uint8_t  songptr;
    uint8_t  pattnum;
    uint32_t pattptr;
    g_player.get_chan_info(selected_chan, songptr, pattnum, pattptr);

    char line[16];
    ivec2 cursor = {0, 32};

    cursor.y += 2;
    // song table
    {
        int len = g_song.songlen[SONG_NR][selected_chan];
        int current_row = std::max<int>(0, songptr - 1);

        int start_row = clamp(len - MAX_SONG_ROWS_ON_SCREEN, 0, current_row - MAX_SONG_ROWS_ON_SCREEN / 2);

        for (int i = 0; i < MAX_SONG_ROWS_ON_SCREEN; ++i) {
            int row = start_row + i;
            if (row >= len) {
                cursor.x = 0;
                cursor.y += ROW_HEIGHT;
                continue;
            }

            // highlight background
            if (row == current_row) {
                dc.set_color(gui::DARK_BLUE);
                dc.box({ cursor - ivec2(0, 2), {8 * 37, ROW_HEIGHT} });
            }
            dc.set_color(gui::LIGHT_GREY);

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
            cursor.y += ROW_HEIGHT;

        }
    }

    cursor.x = 0;
    cursor.y += ROW_HEIGHT;


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
            dc.set_color(gui::DARK_BLUE);
            dc.box({ cursor - ivec2(0, 2), {8 * 37, ROW_HEIGHT} });
        }
        dc.set_color(gui::LIGHT_GREY);

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
                dc.set_color(gui::DARK_GREY);
                dc.text(cursor, "...");
            }
            else if (note == gt::KEYOFF) {
                dc.set_color(gui::LIGHT_GREY);
                dc.text(cursor, "===");
            }
            else if (note == gt::KEYOFF) {
                dc.set_color(gui::LIGHT_GREY);
                dc.text(cursor, "===");
            }
            else {
                dc.set_color(gui::LIGHT_GREY);
                sprintf(line, "%c%c%d", "CCDDEFFGGAAB"[note % 12],
                              "-#-#--#-#-#-"[note % 12],
                              (note - gt::FIRSTNOTE) / 12);
                dc.text(cursor, line);
            }
            cursor.x += 28;

            if (instr == 0) {
                dc.set_color(gui::DARK_GREY);
                dc.text(cursor, "..");
            }
            else {
                dc.set_color(gui::LIGHT_GREY);
                sprintf(line, "%02X", instr);
                dc.text(cursor, line);
            }
            cursor.x += 20;

            if (cmd == 0) {
                dc.set_color(gui::DARK_GREY);
                dc.text(cursor, "...");
            }
            else {
                dc.set_color(gui::LIGHT_GREY);
                sprintf(line, "%X%02X", cmd, arg);
                dc.text(cursor, line);
            }
            cursor.x += 40;
        }
        cursor.x = 0;
        cursor.y += ROW_HEIGHT;
    }


    gfx::draw(dc, g_gui_img);
}




void draw() {
    gfx::set_canvas(g_canvas);
    gfx::set_blend(true);
    gfx::clear(0, 0, 0);

    draw_song();


    // draw canvas
    gfx::set_canvas();
    gfx::set_blend(false);
    gfx::clear(0.1, 0.1, 0.1);
    u8vec4 white(255);
    ivec2 p(g_canvas_offset, 0);
    ivec2 s(W * g_canvas_scale, H * g_canvas_scale);
    ivec2 S(W, H);
    gfx::DrawContext dc;
    dc.add_vertex({p, S.oy(), white});
    dc.add_vertex({ivec2(p.x + s.x, 0), S, white});
    dc.add_vertex({p + s, S.xo(), white});
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
