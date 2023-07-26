#include "project_view.hpp"
#include "gui.hpp"
#include "app.hpp"
#include <array>


namespace project_view {
namespace {


    int g_font;

} // namespace


void init() {


}

void draw() {

    gui::item_size({ 92, 16 });
    gui::text("INPUT");
    gui::same_line();
    gui::item_size({ 33 * 8, 16 });


    // TEST edit instrument name
    gui::input_text(app::song().instr[3].name);


    gui::item_size({ 92, 16 });
    gui::text("FONT");
    gui::same_line();
    gui::item_size({ 32, 16 });
    for (int i = 0; i < 5; ++i) {
        char str[2] = { char('1' + i), '\0' };
        if (gui::button(str, g_font == i)) {
            g_font = i;
            gui::draw_context().font(g_font);
        }
        gui::same_line();
    }
}

} // namespace project_view
