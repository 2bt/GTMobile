#include "app.hpp"
#include "gtsong.hpp"
#include "platform.hpp"
#include "gtplayer.hpp"
#include "sid.hpp"
#include "log.hpp"
#include "gui.hpp"
#include "project_view.hpp"
#include "song_view.hpp"
#include <cstring>



namespace app {
namespace {

gt::Song    g_song;
gt::Player  g_player(g_song);

int         g_canvas_height;
gfx::Canvas g_canvas;
float       g_canvas_scale;
int16_t     g_canvas_offset;
bool        g_initialized = false;



enum class View {
    Project,
    Song,
};

const std::array<const char*, 2> VIEW_NAMES = {
    "PROJECT",
    "SONG",
};

const std::array<void(*)() , 2> VIEW_FUNCS = {
    project_view::draw,
    song_view::draw,
};

View g_view = View::Project;

void set_view(View view) {
    g_view = view;
}


} // namespace


int canvas_height() { return g_canvas_height; }

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

    g_canvas_height = std::max<int16_t>(CANVAS_WIDTH * height / width, CANVAS_MIN_HEIGHT);

    // set canvas offset and scale
    float scale_x = width  / float(CANVAS_WIDTH);
    float scale_y = height / float(g_canvas_height);
    if (scale_y < scale_x) {
        g_canvas_offset = (width - CANVAS_WIDTH * scale_y) * 0.5f;
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
    while (w < CANVAS_WIDTH) w *= 2;
    while (h < g_canvas_height) h *= 2;
    g_canvas.init({ w, h });
    if (g_canvas_scale != (int) g_canvas_scale) {
        g_canvas.set_filter(gfx::Texture::LINEAR);
    }
}


void touch(int x, int y, bool pressed) {
    gui::touch::event((x - g_canvas_offset) / g_canvas_scale, y / g_canvas_scale, pressed);
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
    gui::item_size({80, 32});
    gui::button_style(gui::BoxStyle::Round2);
    for (size_t i = 0; i < VIEW_NAMES.size(); ++i) {
        if (gui::button(VIEW_NAMES[i], i == size_t(g_view))) {
            set_view(View(i));
        }
        gui::same_line();
    }
    gui::button_style(gui::BoxStyle::Normal);

    VIEW_FUNCS[size_t(g_view)]();

    gui::end_frame();


    // draw canvas
    gfx::set_canvas();
    gfx::set_blend(false);
    gfx::clear(0.1, 0.1, 0.1);
    u8vec4 white(255);
    ivec2 p(g_canvas_offset, 0);
    ivec2 S(CANVAS_WIDTH, g_canvas_height);
    ivec2 s = S * g_canvas_scale;
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
