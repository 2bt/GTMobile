#include "project_view.hpp"
#include "gui.hpp"
#include "app.hpp"
#include "log.hpp"
#include "platform.hpp"
#include <array>
#include <filesystem>
#include <cstring>


namespace fs = std::filesystem;


namespace project_view {
namespace {


std::array<char, 32>     g_file_name;
std::vector<std::string> g_file_names;

void draw_load_window() {
    static bool show_load_win = false;

    gui::align(gui::Align::Center);
    gui::item_size({ 92, 16 });
    if (gui::button("LOAD SONG", show_load_win)) {
        show_load_win = true;
        platform::list_assets("songs", g_file_names);
    }
    if (!show_load_win) return;

    gui::begin_window();

    gui::align(gui::Align::Left);
    gui::item_size({ app::CANVAS_WIDTH, 16 });
    for (size_t i = 0; i < g_file_names.size(); ++i) {
        char const* s = g_file_names[i].c_str();
        if (gui::button(s, strcmp(s, g_file_name.data()) == 0)) {
            strncpy(g_file_name.data(), s, g_file_name.size());
        }
    }

    gui::item_size({ app::CANVAS_WIDTH, 24 });
    gui::align(gui::Align::Center);
    if (gui::button("LOAD")) {
        show_load_win = false;

        std::vector<uint8_t> buffer;
        platform::load_asset((std::string("songs/") + g_file_name.data()).c_str(), buffer);
        app::song().load(buffer.data(), buffer.size());
        app::player().init_song(0, gt::Player::PLAY_STOP);
    }
    if (gui::button("CANCEL")) show_load_win = false;

    gui::end_window();
}



} // namespace


void init() {


}

void draw() {

    // gui::item_size({ 92, 16 });
    // gui::text("INPUT");
    // gui::same_line();
    // gui::item_size({ 33 * 8, 16 });
    // gui::input_text(app::song().instr[3].name);


    // gui::item_size({ 92, 16 });
    // gui::text("FONT");
    // gui::same_line();
    // gui::item_size(16);
    // static int font = 0;
    // for (int i = 0; i < 5; ++i) {
    //     char str[2] = { char('1' + i), '\0' };
    //     if (gui::button(str, font == i)) {
    //         font = i;
    //         gui::draw_context().font(font);
    //     }
    //     gui::same_line();
    // }
    // gui::same_line(false);


    draw_load_window();


}

} // namespace project_view
