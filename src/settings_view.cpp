#include "settings_view.hpp"
#include "gui.hpp"
#include "app.hpp"


namespace settings_view {
namespace {
    Settings g_settings {};
}

Settings const& settings() { return g_settings; }
Settings&       mutable_settings() { return g_settings; }

void draw() {

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
    if (gui::button(gui::Icon::Right) && g_settings.row_highlight < 32) ++g_settings.row_highlight;
    gui::same_line();
    gui::item_size({ 12 + 2 * 8, app::BUTTON_WIDTH });
    gui::text("%2d", g_settings.row_highlight);


    gui::item_size({ 12 + 18 * 8, app::BUTTON_WIDTH });
    gui::text("ROW HEIGHT");
    gui::same_line();
    gui::item_size(app::BUTTON_WIDTH);
    if (gui::button(gui::Icon::Left) && g_settings.row_height > 8) --g_settings.row_height;
    gui::same_line();
    if (gui::button(gui::Icon::Right) && g_settings.row_height < 18) ++g_settings.row_height;
    gui::same_line();
    gui::item_size({ 12 + 2 * 8, app::BUTTON_WIDTH });
    gui::text("%2d", g_settings.row_height);
    gui::same_line();
}

} // namespace settings_view


