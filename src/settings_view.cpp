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

    gui::item_size({ app::CANVAS_WIDTH - (12 + 4*8), app::BUTTON_HEIGHT });
    gui::align(gui::Align::Left);
    gui::text("HARD RESTART");
    gui::align(gui::Align::Center);
    gui::same_line();
    gui::item_size({ 12 + 4*8, app::BUTTON_HEIGHT });
    gui::text("%04X", g_song.adparam);

    int a = (g_song.adparam >> 12) & 0xf;
    int d = (g_song.adparam >>  8) & 0xf;
    int s = (g_song.adparam >>  4) & 0xf;
    int r = (g_song.adparam >>  0) & 0xf;
    gui::slider(app::CANVAS_WIDTH, "ATTACK  %X", a, 0, 0xf);
    gui::slider(app::CANVAS_WIDTH, "DECAY   %X", d, 0, 0xf);
    gui::slider(app::CANVAS_WIDTH, "SUSTAIN %X", s, 0, 0xf);
    gui::slider(app::CANVAS_WIDTH, "RELEASE %X", r, 0, 0xf);
    g_song.adparam = (a << 12) | (d << 8) | (s << 4) | r;
    gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
    gui::separator();


    piano::draw();
}

} // namespace settings_view


