#include "app.hpp"
#include "gtsong.hpp"
#include "platform.hpp"
#include "gtplayer.hpp"
#include "sid.hpp"
#include "log.hpp"
#include "gui.hpp"
#include "project_view.hpp"
#include "song_view.hpp"
#include "instrument_view.hpp"
#include <cstring>



namespace app {
namespace {


Settings g_settings {
    .play_in_background = false,
    .row_highlight = 8,
    .row_height = 13,
};


const std::array<const char*, 3> VIEW_NAMES = {
    "PROJECT",
    "SONG",
    "INSTRUMENT",
};

void draw_settings();
const std::array<void(*)(), 4> VIEW_FUNCS = {
    project_view::draw,
    song_view::draw,
    instrument_view::draw,
    draw_settings,
};

View        g_view = View::Project;

gt::Song    g_song;
gt::Player  g_player(g_song);

int         g_canvas_height;
gfx::Canvas g_canvas;
float       g_canvas_scale;
int16_t     g_canvas_offset;
bool        g_initialized = false;



void draw_settings() {

    gui::item_size({ 12 + 18 * 8, app::BUTTON_WIDTH });
    gui::text("PLAY IN BACKGROUND");
    gui::same_line();
    gui::item_size(app::BUTTON_WIDTH);
    if (gui::button(g_settings.play_in_background ? gui::Icon::On : gui::Icon::Off,
                    g_settings.play_in_background)) {
        g_settings.play_in_background ^= 1;
    }

    gui::item_size({ 12 + 18 * 8, app::BUTTON_WIDTH });
    gui::text("ROW HIGHLIGHT STEP");
    gui::same_line();
    gui::item_size(app::BUTTON_WIDTH);
    if (gui::button(gui::Icon::Left) && g_settings.row_highlight > 2) --g_settings.row_highlight;
    gui::same_line();
    gui::item_size({ 12 + 2 * 8, app::BUTTON_WIDTH });
    gui::text("%2d", g_settings.row_highlight);
    gui::same_line();
    gui::item_size(app::BUTTON_WIDTH);
    if (gui::button(gui::Icon::Right) && g_settings.row_highlight < 32) ++g_settings.row_highlight;


    gui::item_size({ 12 + 18 * 8, app::BUTTON_WIDTH });
    gui::text("ROW HEIGHT");
    gui::same_line();
    gui::item_size(app::BUTTON_WIDTH);
    if (gui::button(gui::Icon::Left) && g_settings.row_height > 8) --g_settings.row_height;
    gui::same_line();
    gui::item_size({ 12 + 2 * 8, app::BUTTON_WIDTH });
    gui::text("%2d", g_settings.row_height);
    gui::same_line();
    gui::item_size(app::BUTTON_WIDTH);
    if (gui::button(gui::Icon::Right) && g_settings.row_height < 18) ++g_settings.row_height;
}



} // namespace


Settings const& settings() { return g_settings; }
Settings&       mutable_settings() { return g_settings; }
gt::Song&       song() { return g_song; }
gt::Player&     player() { return g_player; }
int             canvas_height() { return g_canvas_height; }

void set_view(View view) {
    g_view = view;
}


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

    set_view(View::Project);

    g_song.clear();

    g_initialized = true;
}

void free() {
    LOGD("free");
    gfx::free();
    gui::free();
    g_canvas.free();
}

void resize(int width, int height) {
    gfx::screen_size({width, height});

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
    // g_canvas_scale = 1;
    LOGD("canvas scale = %f", g_canvas_scale);

    // reinit canvas with POT dimensions
    int w = 2;
    int h = 2;
    while (w < CANVAS_WIDTH) w *= 2;
    while (h < g_canvas_height) h *= 2;
    g_canvas.init({ w, h });
    if (g_canvas_scale != (int) g_canvas_scale) {
        g_canvas.filter(gfx::Texture::LINEAR);
    }
}


void touch(int x, int y, bool pressed) {
    gui::touch_event((x - g_canvas_offset) / g_canvas_scale, y / g_canvas_scale, pressed);
}

void key(int key, int unicode) {
    LOGD("key %d %d", key, unicode);
    gui::key_event(key, unicode);
}



void draw() {
    sid::update_state();

    gfx::canvas(g_canvas);
    gfx::blend(true);
    gfx::clear(0.0, 0.0, 0.0);

    gui::begin_frame();

    gui::item_size({ 112, 24 });
    gui::align(gui::Align::Center);
    gui::button_style(gui::ButtonStyle::Tab);
    for (size_t i = 0; i < VIEW_NAMES.size(); ++i) {
        if (gui::button(VIEW_NAMES[i], i == size_t(g_view))) {
            set_view(View(i));
        }
        gui::same_line();
    }
    gui::item_size(24);
    if (gui::button(gui::Icon::Settings, g_view == View::Settings)) {
        set_view(View::Settings);
    }
    gui::button_style(gui::ButtonStyle::Normal);

    VIEW_FUNCS[size_t(g_view)]();


    gui::end_frame();


    // draw canvas
    gfx::reset_canvas();
    gfx::blend(false);
    gfx::clear(0.1, 0.1, 0.1);
    u8vec4 white(255);
    ivec2 p(g_canvas_offset, 0);
    ivec2 S(CANVAS_WIDTH, g_canvas_height);
    ivec2 s = S * g_canvas_scale;
    gfx::Mesh mesh;
    mesh.vertices = {
        { p, S.oy(), white },
        { ivec2(p.x + s.x, 0), S, white },
        { p + s, S.xo(), white },
        { ivec2{ p.x, s.y }, {}, white },
    };
    mesh.indices = { 0, 1, 2, 0, 2, 3 };
    gfx::draw(mesh, g_canvas);
}


} // namespace app
