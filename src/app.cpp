#include "app.hpp"
#include "gtsong.hpp"
#include "platform.hpp"
#include "gtplayer.hpp"
#include "sid.hpp"
#include "log.hpp"
#include "project_view.hpp"
#include "song_view.hpp"
#include "instrument_view.hpp"
#include "instrument_manager_view.hpp"
#include "settings_view.hpp"
#include <cstring>




namespace app {
namespace {

enum class View {
    Project,
    Song,
    Instrument,
    InstrumentManager,
    Settings,
};

View        g_view = View::Project;
gt::Song    g_song;
gt::Player  g_player(g_song);

int         g_canvas_height;
gfx::Canvas g_canvas;
float       g_canvas_scale;
int16_t     g_canvas_offset;
bool        g_initialized = false;
std::string g_storage_dir = ".";

ConfirmCallback g_confirm_callback;
std::string     g_confirm_msg;


} // namespace


gt::Song&          song() { return g_song; }
gt::Player&        player() { return g_player; }
int                canvas_height() { return g_canvas_height; }
std::string const& storage_dir() { return g_storage_dir; }
void set_storage_dir(std::string const& storage_dir) { g_storage_dir = storage_dir; }

void draw_confirm() {
    if (g_confirm_msg.empty()) return;
    gui::Box box = gui::begin_window({ app::CANVAS_WIDTH - 24 * 2, app::BUTTON_HEIGHT * 2 });

    gui::item_size({ box.size.x, app::BUTTON_HEIGHT });
    gui::align(gui::Align::Center);
    gui::text(g_confirm_msg.c_str());
    if (!g_confirm_callback) {
        if (gui::button("OK")) g_confirm_msg.clear();
        return;
    }
    gui::item_size({ box.size.x / 2, app::BUTTON_HEIGHT });
    if (gui::button("OK")) {
        g_confirm_msg.clear();
        g_confirm_callback(true);
    }
    gui::same_line();
    if (gui::button("CANCEL")) {
        g_confirm_msg.clear();
        g_confirm_callback(false);
    }
    gui::end_window();
}
void confirm(std::string msg, ConfirmCallback cb) {
    g_confirm_msg      = std::move(msg);
    g_confirm_callback = std::move(cb);
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
    song_view::reset();
    project_view::init();
    g_song.clear();

    // simple beep instrument
    strcpy(g_song.instruments[1].name.data(), "BEEP");
    g_song.instruments[1].sr = 0xf3;
    g_song.instruments[1].ptr[0] = 1;
    g_song.ltable[0][0] = 0x11;
    g_song.ltable[0][1] = 0xff;

    g_initialized = true;
}

void free() {
    LOGD("free");
    gfx::free();
    gui::free();
    g_canvas.free();
}

void resize(int width, int height) {
    width  = std::max(1, width);
    height = std::max(1, height);
    gfx::screen_size({ width, height });

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
    gui::key_event(key, unicode);
}



void draw() {
    sid::update_state();

    gfx::canvas(g_canvas);
    gfx::blend(true);
    gfx::clear(0.0, 0.0, 0.0);

    gui::begin_frame();

    gui::item_size({ (CANVAS_WIDTH - TAB_HEIGHT) / 3 , TAB_HEIGHT });
    gui::align(gui::Align::Center);
    gui::button_style(gui::ButtonStyle::Tab);
    if (gui::button("PROJECT", g_view == View::Project)) {
        g_view = View::Project;
        project_view::init();
    }
    gui::same_line();
    if (gui::button("SONG", g_view == View::Song)) {
        g_view = View::Song;
    }
    gui::same_line();
    if (gui::button("INSTRUMENT", g_view == View::Instrument || g_view == View::InstrumentManager)) {
        if (g_view == View::Instrument) {
            g_view = View::InstrumentManager;
            instrument_manager_view::init();
        }
        else {
            g_view = View::Instrument;
        }
    }
    gui::same_line();
    gui::item_size({ CANVAS_WIDTH - gui::cursor().x, TAB_HEIGHT });
    if (gui::button(gui::Icon::Settings, g_view == View::Settings)) {
        g_view = View::Settings;
    }
    gui::item_size({ CANVAS_WIDTH, BUTTON_HEIGHT });
    gui::separator();
    gui::button_style(gui::ButtonStyle::Normal);

    switch (g_view) {
    case View::Project: project_view::draw(); break;
    case View::Song: song_view::draw(); break;
    case View::Instrument: instrument_view::draw(); break;
    case View::InstrumentManager: instrument_manager_view::draw(); break;
    case View::Settings: settings_view::draw(); break;
    }


    gui::end_frame();


    // draw canvas
    gfx::reset_canvas();
    gfx::blend(false);
    gfx::clear(0.1, 0.1, 0.1);
    u8vec4 white(255);
    ivec2 p(g_canvas_offset, 0);
    ivec2 S(CANVAS_WIDTH, g_canvas_height);
    ivec2 s = S * g_canvas_scale;
    static gfx::Mesh mesh;
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
