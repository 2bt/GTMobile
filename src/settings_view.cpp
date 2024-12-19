#include "settings_view.hpp"
#include "app.hpp"
#include "piano.hpp"


namespace settings_view {
namespace {

gt::Song& g_song = app::song();
Settings  g_settings {};

} // namespace

Settings const& settings() { return g_settings; }
Settings&       mutable_settings() { return g_settings; }

void draw() {

    gui::slider(app::CANVAS_WIDTH, "ROW HIGHLIGHT %02X", g_settings.row_highlight, 2, 16);
    gui::slider(app::CANVAS_WIDTH, "ROW HEIGHT    %02X", g_settings.row_height, 8, app::MAX_ROW_HEIGHT);

    gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
    gui::separator();

    int a = (g_song.adparam >> 12) & 0xf;
    int d = (g_song.adparam >>  8) & 0xf;
    int s = (g_song.adparam >>  4) & 0xf;
    int r = (g_song.adparam >>  0) & 0xf;
    gui::slider(app::CANVAS_WIDTH, "HR ATTACK  %X", a, 0, 0xf);
    gui::slider(app::CANVAS_WIDTH, "HR DECAY   %X", d, 0, 0xf);
    gui::slider(app::CANVAS_WIDTH, "HR SUSTAIN %X", s, 0, 0xf);
    gui::slider(app::CANVAS_WIDTH, "HR RELEASE %X", r, 0, 0xf);
    g_song.adparam = (a << 12) | (d << 8) | (s << 4) | r;
    gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });

    // SID model


    // multiplier
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

    gui::item_size({ 12 * 8 + 12, app::BUTTON_HEIGHT });
    gui::text("SID MODEL   ");
    gui::same_line();
    gui::item_size({ (app::CANVAS_WIDTH - gui::cursor().x) / 2, app::BUTTON_HEIGHT });
    gui::button_style(gui::ButtonStyle::RadioLeft);
    if (gui::button("6581", g_song.model == gt::Model::MOS6581)) {
        g_song.model = gt::Model::MOS6581;
        app::sid().init(Sid::Model::MOS6581, Sid::SamplingMethod::Fast);
    }
    gui::same_line();
    gui::button_style(gui::ButtonStyle::RadioRight);
    if (gui::button("8580", g_song.model == gt::Model::MOS8580)) {
        g_song.model = gt::Model::MOS8580;
        app::sid().init(Sid::Model::MOS8580, Sid::SamplingMethod::Fast);
    }

    piano::draw();
}

} // namespace settings_view


