#include "project_view.hpp"
#include "gui.hpp"
#include "app.hpp"
#include <array>


namespace project_view {
namespace {


    std::array<char, 32> g_str;

    int g_font;

} // namespace


void init() {


}

void draw() {

    gui::item_size({ 92, 16 });
    gui::text("INPUT");
    gui::same_line();
    gui::item_size({ 33 * 8, 16 });


    // gui::input_text(g_str);
    gui::input_text(app::song().instr[3].name);


    gui::item_size({ 92, 16 });
    gui::text("FONT");
    gui::same_line();
    gui::item_size({ 65, 16 });
    if (gui::button("C64",    g_font == 0)) g_font = 0;
    gui::same_line();
    if (gui::button("Fake",   g_font == 1)) g_font = 1;
    gui::same_line();
    if (gui::button("ZX",     g_font == 2)) g_font = 2;
    gui::same_line();
    if (gui::button("FOOBAR", g_font == 3)) g_font = 3;
    gui::draw_context().font(g_font);
}

} // namespace project_view
