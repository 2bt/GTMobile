#include "settings_view.hpp"
#include "app.hpp"
#include "piano.hpp"
#include "platform.hpp"

namespace settings_view {
namespace {

gt::Song& g_song = app::song();
Settings  g_settings {};

enum {
    #define X(n, ...) SETTING_##n,
    SETTINGS(X)
    #undef X
};

} // namespace

Settings const& settings() { return g_settings; }
Settings&       mutable_settings() { return g_settings; }


// functions for loading and storing settings in android
char const* get_setting_name(int i) {
    switch (i) {
    #define X(n, ...) case SETTING_##n: return #n;
    SETTINGS(X)
    #undef X
    default: return nullptr;
    }
}
int get_setting_value(int i) {
    switch (i) {
    #define X(n, ...) case SETTING_##n: return g_settings.n;
    SETTINGS(X)
    #undef X
    default: return 0;
    }
}
void set_setting_value(int i, int v) {
    switch (i) {
    #define X(n, ...) case SETTING_##n: g_settings.n = v; break;
    SETTINGS(X)
    #undef X
    default: break;
    }
}



void draw() {


    enum class Mode { Project, Editor };
    enum class Window {
        None,
        SamplingMethod,
        HardRestart,
    };
    static Mode   mode   = Mode::Project;
    static Window window = Window::None;

    constexpr char const* SAMPLING_LABELS[] = {
        "FAST",
        "INTERPOLATE",
        "RESAMPLE INTERPOLATE",
        "RESAMPLE FAST",
    };


    gui::button_style(gui::ButtonStyle::Tab);
    gui::item_size({ app::CANVAS_WIDTH / 2, app::BUTTON_HEIGHT });
    if (gui::button("PROJECT SETTINGS", mode == Mode::Project)) mode = Mode::Project;
    gui::same_line();
    if (gui::button("EDITOR SETTINGS", mode == Mode::Editor)) mode = Mode::Editor;
    gui::button_style(gui::ButtonStyle::Normal);
    gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
    gui::separator();

    if (mode == Mode::Project) {

        gui::choose(app::CANVAS_WIDTH, "CHIP MODEL  ", g_song.model, { "6581", "8580" });

        // speed/multiplier
        char str[32] = "SPEED   25Hz";
        if (g_song.multiplier > 0) {
            sprintf(str, "SPEED   %3dX", g_song.multiplier);
        }
        if (gui::slider(app::CANVAS_WIDTH, str, g_song.multiplier, 0, 16)) {
            // update gatetimer of unused instruments
            for (gt::Instrument& instr : g_song.instruments) {
                bool set = instr.ptr[gt::WTBL] | instr.ptr[gt::PTBL] | instr.ptr[gt::FTBL];
                if (!set) instr.gatetimer = 2 * g_song.multiplier;
            }
        }

        // hard restart
        gui::item_size({ 12 + 8 * 12, app::BUTTON_HEIGHT });
        gui::align(gui::Align::Left);
        gui::text("HARD RESTART");
        gui::same_line();
        gui::align(gui::Align::Center);
        sprintf(str, "%04X", g_song.adparam);
        gui::item_size({ app::CANVAS_WIDTH - gui::cursor().x, app::BUTTON_HEIGHT });
        if (gui::button(str)) {
            window = Window::HardRestart;
        }
        gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });

    }
    else if (mode == Mode::Editor) {
        if (gui::choose(app::CANVAS_WIDTH, "FULLSCREEN     ", g_settings.fullscreen_enabled)) {
            platform::update_setting(SETTING_fullscreen_enabled);
        }
        if (gui::choose(app::CANVAS_WIDTH, "KEEP SCREEN ON ", g_settings.keep_screen_on)) {
            platform::update_setting(SETTING_keep_screen_on);
        }

        gui::slider(app::CANVAS_WIDTH, "ROW HEIGHT   %02X", g_settings.row_height, 8, app::MAX_ROW_HEIGHT);
        gui::slider(app::CANVAS_WIDTH, "HIGHLIGHT    %02X", g_settings.row_highlight, 2, 16);

        gui::choose(app::CANVAS_WIDTH, "REG WRITE ORDER", g_settings.register_write_order, { "v2.68", "v2.73" });

        // sampling method
        gui::item_size({ 12 + 8 * 15, app::BUTTON_HEIGHT });
        gui::text("SAMPLING METHOD");
        gui::same_line();
        gui::item_size({ app::CANVAS_WIDTH - gui::cursor().x, app::BUTTON_HEIGHT });
        if (gui::button(SAMPLING_LABELS[g_settings.sampling_method])) {
            window = Window::SamplingMethod;
        }
    }


    if (window == Window::SamplingMethod) {
        gui::Box box = gui::begin_window({ app::CANVAS_WIDTH - 48, app::BUTTON_HEIGHT * 6 + gui::FRAME_WIDTH * 2 });
        gui::item_size({ box.size.x, app::BUTTON_HEIGHT });
        gui::text("SAMPLING METHOD");
        gui::separator();
        for (int i = 0; i < 4; ++i) {
            if (gui::button(SAMPLING_LABELS[i], i == g_settings.sampling_method)) {
                g_settings.sampling_method = i;
                window = Window::None;
            }
        }
        gui::separator();
        if (gui::button("CLOSE")) {
            window = Window::None;
        }
        gui::end_window();
    }
    else if (window == Window::HardRestart) {
        gui::Box box = gui::begin_window({ app::CANVAS_WIDTH - 48, app::BUTTON_HEIGHT * 6 + gui::FRAME_WIDTH * 2 });
        gui::item_size({ box.size.x, app::BUTTON_HEIGHT });
        gui::text("HARD RESTART");
        gui::separator();
        constexpr char const* LABELS[] = { "ATTACK  %X", "DECAY   %X", "SUSTAIN %X", "RELEASE %X" };
        int adsr[] = {
            (g_song.adparam >> 12) & 0xf,
            (g_song.adparam >>  8) & 0xf,
            (g_song.adparam >>  4) & 0xf,
            (g_song.adparam >>  0) & 0xf,
        };
        for (int i = 0; i < 4; ++i) {
            gui::slider(box.size.x, LABELS[i], adsr[i], 0, 0xf);
        }
        g_song.adparam = (adsr[0] << 12) | (adsr[1] << 8) | (adsr[2] << 4) | adsr[3];
        gui::item_size({ box.size.x, app::BUTTON_HEIGHT });
        gui::separator();
        if (gui::button("CLOSE")) {
            window = Window::None;
        }
        gui::end_window();
    }

    piano::draw();
}

} // namespace settings_view


