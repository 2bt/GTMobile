#include "settings_view.hpp"
#include "gui.hpp"
#include "app.hpp"
#include "piano.hpp"


namespace settings_view {
namespace {
    Settings g_settings {};
}

Settings const& settings() { return g_settings; }
Settings&       mutable_settings() { return g_settings; }

void draw() {
    enum {
        C1W = 12 + 21 * 8,
        C2W = app::CANVAS_WIDTH - C1W,
    };

    gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
    if (gui::button("PLAY IN BACKGROUND", g_settings.play_in_background)) {
        g_settings.play_in_background ^= 1;
    }

    gui::item_size({ C1W , app::BUTTON_HEIGHT });
    gui::text("ROW HIGHLIGHT STEP %02X", g_settings.row_highlight);
    gui::same_line();
    app::slider(C2W, g_settings.row_highlight, 2, 16);

    gui::item_size({ C1W , app::BUTTON_HEIGHT });
    gui::text("ROW HEIGHT         %02X", g_settings.row_height);
    gui::same_line();
    app::slider(C2W, g_settings.row_height, 8, app::MAX_ROW_HEIGHT);

    piano::draw();
}

} // namespace settings_view


