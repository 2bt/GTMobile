#include "app.hpp"
#include "gtsong.hpp"
#include "gui.hpp"
#include "platform.hpp"
#include "gtplayer.hpp"
#include "sid.hpp"
#include "log.hpp"
#include "project_view.hpp"
#include "song_view.hpp"
#include "instrument_view.hpp"
#include "instrument_manager_view.hpp"
#include "command_edit.hpp"
#include "settings_view.hpp"
#include <cstring>
#include <fstream>




namespace app {
namespace {

enum class View {
    Splash,
    Project,
    Song,
    Pattern,
    Instrument,
    InstrumentManager,
    Settings,
};

gt::Song        g_song;
gt::Player      g_player(g_song);
Sid             g_sid;
int             g_canvas_height;
gfx::Canvas     g_canvas;
float           g_canvas_scale;
int16_t         g_canvas_offset;
ConfirmCallback g_confirm_callback;
std::string     g_confirm_msg;
View            g_view        = View::Splash;
bool            g_initialized = false;
std::string     g_storage_dir = ".";
Mixer           g_mixer(g_player, g_sid);
bool            g_take_screenshot = false;


void draw_play_buttons() {

    gui::cursor({ 0, canvas_height() - TAB_HEIGHT - gui::FRAME_WIDTH });
    gui::item_size(CANVAS_WIDTH);
    gui::separator();
    gui::item_size(TAB_HEIGHT);

    static float backward_time = 0.0f;
    backward_time += gui::frame_time();
    if (gui::button(gui::Icon::FastBackward)) {
        if (g_player.is_playing()) {
            g_player.set_action(backward_time > 0.5f ? gt::Player::Action::RestartPattern
                                                     : gt::Player::Action::FastBackward);
        }
        else {
            g_player.m_start_patt_pos = {};
            if (g_player.m_current_patt_pos == std::array<int, 3>{}) {
                for (int& x : g_player.m_current_song_pos) {
                    if (x > 0) --x;
                }
            }
            else {
                g_player.m_current_patt_pos = {};
            }
            g_player.m_start_song_pos = g_player.m_current_song_pos;
        }
        backward_time = 0.0f;
    }
    gui::same_line();

    if (gui::button(gui::Icon::Stop)) {
        g_player.m_start_song_pos.fill(song_view::song_position());
        g_player.m_start_patt_pos = {};
        g_player.set_action(gt::Player::Action::Stop);
    }


    // play button
    int w = CANVAS_WIDTH - TAB_HEIGHT * 5;
    gui::item_size({ w, TAB_HEIGHT });
    bool is_playing = g_player.is_playing();
    gui::same_line();
    if (gui::button(gui::Icon::PlayPause, is_playing)) {
        g_player.set_action(is_playing ? gt::Player::Action::Pause : gt::Player::Action::Start);
    }

    gui::item_size(TAB_HEIGHT);
    gui::same_line();
    if (gui::button(gui::Icon::Follow, song_view::get_follow())) {
        song_view::toggle_follow();
    }
    gui::same_line();

    bool loop = g_player.get_pattern_looping();
    if (gui::button(gui::Icon::Loop, loop)) {
        g_player.set_pattern_loopping(!loop);
    }

    gui::same_line();
    if (gui::button(gui::Icon::FastForward)) {
        if (g_player.is_playing()) {
            g_player.set_action(gt::Player::Action::FastForward);
        }
        else {
            g_player.m_start_patt_pos   = {};
            g_player.m_current_patt_pos = {};

            for (int& x : g_player.m_current_song_pos) {
                if (x < g_song.song_len - 1) ++x;
                else x = 0;
            }
            g_player.m_start_song_pos = g_player.m_current_song_pos;
        }
    }
}


void draw_splash() {
    gui::begin_window({ CANVAS_WIDTH, canvas_height() });
    int cy = canvas_height() / 2;
    gui::DrawContext& dc = gui::draw_context();
    dc.rgb(color::WHITE);
    cy -= 114;
    dc.rect({ 52, cy}, { 256, 176 }, { 0, 336 });
    dc.text({ 60, cy + 176 + 24 }, "CREATE AUTHENTIC C64 SID MUSIC");
    dc.text({ 60, cy + 176 + 40 }, "          ON THE GO!");

    if (gui::touch::just_released()) {
        g_view = View::Project;
        project_view::init();
    }

    gui::end_window();
}


} // namespace


gt::Song&          song() { return g_song; }
gt::Player&        player() { return g_player; }
Sid&               sid() { return g_sid; }
int                canvas_height() { return g_canvas_height; }
std::string const& storage_dir() { return g_storage_dir; }
void               set_storage_dir(std::string const& storage_dir) { g_storage_dir = storage_dir; }
bool               is_in_song_view() { return g_view == View::Song; }
void               go_to_instrument_view() { g_view = View::Instrument; }

void draw_confirm() {
    if (g_confirm_msg.empty()) return;
    gui::Box box = gui::begin_window({ CANVAS_WIDTH - 48, BUTTON_HEIGHT * 2 + gui::FRAME_WIDTH });

    gui::item_size({ box.size.x, BUTTON_HEIGHT });
    gui::align(gui::Align::Center);
    gui::text(g_confirm_msg.c_str());
    gui::separator();
    if (!g_confirm_callback) {
        if (gui::button("OK")) g_confirm_msg.clear();
        return;
    }
    gui::item_size({ box.size.x / 2, BUTTON_HEIGHT });
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


void Mixer::mix(int16_t* buffer, int length) {
    enum {
        REG_COUNT        = 25,
        REG_WRITE_CYCLES = 14,
    };

    // NOTE: this is an extract from the GoatTracker changelog
    // 15 January 2009
    // v2.68
    //           - SID register write order tweaked to resemble JCH NewPlayer 21.
    //           - Unbuffered playroutine optimized & modified to resemble buffered
    //             mode timing more.
    // ...
    // 23 July 2014
    // v2.73     - Reverted to old playroutine timing.
    //           - Added full buffering option (similar to ZP ghostregs) which will
    //             buffer everything to ghost regs and dump the previous frame to the
    //             SID at the beginning of the play call
    constexpr uint8_t REG_ORDERS[2][2][REG_COUNT] = {
        // v2.68
        {
            {
                0x15, 0x16, 0x18, 0x17,                   //
                0x05, 0x06, 0x02, 0x03, 0x00, 0x01, 0x04, //
                0x0c, 0x0d, 0x09, 0x0a, 0x07, 0x08, 0x0b, //
                0x13, 0x14, 0x10, 0x11, 0x0e, 0x0f, 0x12, //
            },
            {
                0x15, 0x16, 0x18, 0x17,                   //
                0x04, 0x00, 0x01, 0x02, 0x03, 0x05, 0x06, //
                0x0b, 0x07, 0x08, 0x09, 0x0a, 0x0c, 0x0d, //
                0x12, 0x0e, 0x0f, 0x10, 0x11, 0x13, 0x14, //
            },
        },
        // v2.73
        {
            {
                0x18, 0x17, 0x16, 0x15,                   //
                0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, //
                0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, //
                0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00, //
            },
            {
                0x00, 0x01, 0x02, 0x03,                   //
                0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, //
                0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, //
                0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, //
            },
        },
    };
    uint8_t const* reg_order =
        REG_ORDERS[settings_view::settings().register_write_order][m_player.song().adparam >= 0xf000];

    int const  ticks_per_second  = m_player.song().multiplier * 50 ?: 25;
    int const  cycles_per_tick   = Sid::CLOCKRATE_PAL / ticks_per_second;
    int        cycles_left       = length * uint64_t(Sid::CLOCKRATE_PAL) / MIXRATE;
    while (cycles_left > 0) {
        if (m_cycles_to_next_write == 0) {
            if (m_reg == 0) m_player.play_routine();
            // write register
            int r = reg_order[m_reg];
            m_sid.set_reg(r, m_player.registers()[r]);
            // next register
            if (++m_reg >= REG_COUNT) {
                m_reg = 0;
                m_cycles_to_next_write = cycles_per_tick - REG_WRITE_CYCLES * (REG_COUNT - 1);
            }
            else {
                m_cycles_to_next_write = REG_WRITE_CYCLES;
            }
        }
        int c = std::min(cycles_left, m_cycles_to_next_write);
        cycles_left -= c;
        m_cycles_to_next_write -= c;
        int n = m_sid.clock(c, buffer, length);
        buffer += n;
        length -= n;
    }
    // sometimes there's a sample left that needs rendering
    assert(length <= 1);
    if (length > 0) {
        m_sid.clock(9999, buffer, length);
    }
}


void audio_callback(int16_t* buffer, int length) {
    if (!g_initialized) {
        memset(buffer, 0, sizeof(int16_t) * length);
        return;
    }

    // update sid settings
    static int chip_model      = int(g_song.model);
    static int sampling_method = settings_view::settings().sampling_method;

    if (chip_model != int(g_song.model)) {
        chip_model = int(g_song.model);
        g_sid.set_chip_model(Sid::Model(chip_model));
    }
    if (sampling_method != settings_view::settings().sampling_method) {
        sampling_method = settings_view::settings().sampling_method;
        g_sid.set_sampling_method(Sid::SamplingMethod(sampling_method));
    }

    g_mixer.mix(buffer, length);
}

void reset() {
    g_initialized = false;
    g_view        = View::Splash;
    project_view::reset();
    song_view::reset();
    instrument_view::reset();
    instrument_manager_view::reset();
    command_edit::reset();
    g_song.clear();
    g_sid.init(Sid::Model::MOS8580, Sid::SamplingMethod::Fast);
    g_player.set_action(gt::Player::Action::Reset);
}

void init() {
    LOGD("app::init");
    reset();
    // simple beep instrument
    strcpy(g_song.instruments[1].name.data(), "Beep");
    g_song.instruments[1].sr = 0xf3;
    g_song.instruments[1].ptr[0] = 1;
    g_song.ltable[0][0] = 0x11;
    g_song.ltable[0][1] = 0xff;

    gfx::init();
    gui::init();
    project_view::init();

    g_initialized = true;
}

void free() {
    LOGD("app::free");
    gfx::free();
    gui::free();
    g_canvas.free();
    reset();
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

    // g_canvas_scale = 1;

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
    if (key == gui::KEYCODE_PRINTSCREEN) {
        g_take_screenshot = true;
    }
}



void draw() {
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
    if (gui::button("SONG", g_view == View::Song || g_view == View::Pattern)) {
        // if (g_view == View::Song) {
        //     g_view = View::Pattern;
        // }
        // else {
            g_view = View::Song;
        // }
    }
    gui::same_line();

    gui::ColorTheme old_theme = gui::color_theme();
    if (g_view == View::Instrument) {
        gui::color_theme().button_normal = color::BUTTON_ACTIVE;
        gui::color_theme().button_pressed = color::BUTTON_ALT_PRESSED;
    }
    else if (g_view == View::InstrumentManager) {
        gui::color_theme().button_normal = color::BUTTON_ALT_ACTIVE;
    }
    if (gui::button("INSTRUMENT")) {
        if (g_view == View::Instrument) {
            g_view = View::InstrumentManager;
            instrument_manager_view::init();
        }
        else {
            g_view = View::Instrument;
        }
    }
    gui::color_theme() = old_theme;


    gui::same_line();
    gui::item_size({ CANVAS_WIDTH - gui::cursor().x, TAB_HEIGHT });
    if (gui::button(gui::Icon::Settings, g_view == View::Settings)) {
        g_view = View::Settings;
    }
    gui::item_size({ CANVAS_WIDTH, BUTTON_HEIGHT });
    gui::separator();
    gui::button_style(gui::ButtonStyle::Normal);

    switch (g_view) {
    case View::Splash: draw_splash(); break;
    case View::Project: project_view::draw(); break;
    case View::Song: song_view::draw(); break;
    case View::Pattern: song_view::draw_pattern(); break;
    case View::Instrument: instrument_view::draw(); break;
    case View::InstrumentManager: instrument_manager_view::draw(); break;
    case View::Settings: settings_view::draw(); break;
    }
    draw_play_buttons();
    gui::end_frame();


    // draw canvas
    gfx::reset_canvas();
    gfx::blend(false);
    gfx::clear(0.0, 0.0, 0.0);
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

#ifndef ANDROID
    if (g_take_screenshot) {
        g_take_screenshot = false;
        static int count = 0;
        LOGI("screenshot %d", count);
        char str[32];
        sprintf(str, "shot-%04d.ppm", count);
        ++count;

        std::vector<uint8_t> data;
        g_canvas.get_pixel_data(data);
        std::ofstream file(str, std::ios::binary);
        file << "P6 " << CANVAS_WIDTH << " " << g_canvas_height << " 255\n";
        for (int y = 0; y < g_canvas_height; ++y) {
            for (int x = 0; x < CANVAS_WIDTH; ++x) {
                file.write((char const*) data.data() + 3 * (y * g_canvas.size().x + x), 3);
            }
        }
        file.close();
    }
#endif
}


} // namespace app
