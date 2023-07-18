#include "app.hpp"
#include "gtsong.hpp"
#include "platform.hpp"
#include "gtplayer.hpp"
#include "sid.hpp"
#include "log.hpp"
#include "gui.hpp"
#include "songview.hpp"
#include <cstring>



namespace app {
namespace {

gt::Song    g_song;
gt::Player  g_player(g_song);

gfx::Canvas g_canvas;
float       g_canvas_scale;
int16_t     g_canvas_offset;
bool        g_initialized = false;

int         W, H;

} // namespace


gt::Song& song() { return g_song; }
gt::Player& player() { return g_player; }


void audio_callback(int16_t* buffer, int length) {
    if (!g_initialized) {
        memset(buffer, 0, sizeof(int16_t) * length);
        return;
    }

    enum {
        SAMPLES_PER_FRAME = MIXRATE / 50,
    };
    static int sample = 0;
    while (length > 0) {
        if (sample == 0) {
            g_player.play_routine();
            for (int i = 0; i < 25; ++i) sid::write(i, g_player.regs[i]);
        }
        int l = std::min(SAMPLES_PER_FRAME - sample, length);
        sample += l;
        if (sample == SAMPLES_PER_FRAME) sample = 0;
        length -= l;
        sid::mix(buffer, l);
        buffer += l;
    }
}

void init() {
    LOGD("init");

    sid::init(MIXRATE);
    gfx::init();
    gui::init();


    // load song
    std::vector<uint8_t> buffer;
    platform::load_asset("Smoke_and_Mirrors.sng", buffer);
//    platform::load_asset("Nordic_Scene_Review.sng", buffer);
//    platform::load_asset("test.sng", buffer);
    struct MemBuf : std::streambuf {
        MemBuf(uint8_t const* data, size_t size) {
            char* p = (char*) data;
            this->setg(p, p, p + size);
        }
    } membuf(buffer.data(), buffer.size());
    std::istream stream(&membuf);
    g_song.load(stream);
    g_player.init_song(0, gt::Player::PLAY_STOP);

//    g_player.init_song(0, gt::Player::PLAY_BEGINNING);

    g_initialized = true;
}

void free() {
    LOGD("free");
    gfx::free();
    gui::free();
    g_canvas.free();
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

    // XXX
    //g_canvas_scale = 1;

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



void draw() {
    sid::update_state();

    gfx::set_canvas(g_canvas);
    gfx::set_blend(true);
    gfx::clear(0, 0, 0);


    gui::begin_frame();
    songview::draw();
    gui::end_frame();


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
