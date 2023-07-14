#include "app.hpp"
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

gfx::Image  g_gui_image;
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
    g_gui_image.init("gui.png");



    // load song
    std::vector<uint8_t> buffer;
    platform::load_asset("Remembrance.sng", buffer);
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
    g_gui_image.free();
}

void resize(int width, int height) {
    LOGD("resize %d %d", width, height);
    gfx::resize(width, height);



    W = WIDTH;
    H = std::max<int16_t>(WIDTH * height / width, MIN_HEIGHT);
    // reinit canvas with POT dimensions
    {
        int w = 2;
        int h = 2;
        while (w < W) w *= 2;
        while (h < H) h *= 2;
        g_canvas.init(w, h);
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


int tick = 0;

void draw() {
    tick += 1;

    gfx::set_canvas(g_canvas);
    gfx::set_blend(true);
    gfx::clear(0.1f, 0.3f, 0.2f);

    gfx::DrawContext dc;
    dc.rect(gfx::Vec(100 + sin(tick * 0.01) * 100, 100 + sin(tick * 0.037) * 32),
            {32, 32},
            {0, 0},
            {255});

    gfx::draw(dc, g_gui_image);





    // draw canvas
    gfx::set_canvas();
    gfx::set_blend(false);
    gfx::clear(0, 0, 0);
    gfx::Col white(255);
    using Vec = gfx::Vec;
    Vec cs(W, H);
    Vec p(g_canvas_offset, 0);
    Vec s(cs.x * g_canvas_scale, cs.y * g_canvas_scale);
    dc.clear();
    dc.add_vertex({p, {0, cs.y}, white});
    dc.add_vertex({Vec(p.x + s.x, 0), cs, white});
    dc.add_vertex({p + s, {cs.x, 0}, white});
    dc.add_vertex({{p.x, s.y}, {0, 0}, white});
    dc.add_index(0);
    dc.add_index(1);
    dc.add_index(2);
    dc.add_index(0);
    dc.add_index(2);
    dc.add_index(3);
    gfx::draw(dc, g_canvas);
}


} // namespace app
