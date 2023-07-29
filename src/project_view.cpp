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
    gui::item_size({ app::CANVAS_WIDTH, 16 });
    if (gui::button("LOAD SONG", show_load_win)) {
        show_load_win = true;
        platform::list_assets("songs", g_file_names);
    }
    if (!show_load_win) return;


    gui::begin_window();


    enum {
        PAGE = 20,
    };
    static int scroll = 0;

    ivec2 p = gui::cursor();
    gui::align(gui::Align::Left);
    gui::item_size({ app::CANVAS_WIDTH - 16, 16 });

    for (int i = 0; i < PAGE; ++i) {
        int row = scroll + i;
        if (row >= g_file_names.size()) {
            gui::item_box();
            continue;
        }
        char const* s = g_file_names[row].c_str();
        if (gui::button(s, strcmp(s, g_file_name.data()) == 0)) {
            strncpy(g_file_name.data(), s, g_file_name.size() - 1);
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

    // scrollbar
    gui::cursor(ivec2(app::CANVAS_WIDTH - 16, p.y));
    gui::item_size({ 16, PAGE * 16 });
    int max_scroll = std::max(0, int(g_file_names.size()) - PAGE);
    gui::vertical_drag_bar(scroll, 0, max_scroll, PAGE);


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


    enum {
        INPUT_WIDTH = 33 * 8,
    };


    gt::Song& song = app::song();

    gui::item_size(16);
    gui::item_box();

    gui::align(gui::Align::Left);
    gui::item_size({ app::CANVAS_WIDTH - INPUT_WIDTH, 16 });
    gui::text("NAME");
    gui::same_line();
    gui::item_size({ INPUT_WIDTH, 16 });
    gui::input_text(song.songname);

    gui::item_size({ app::CANVAS_WIDTH - INPUT_WIDTH, 16 });
    gui::text("AUTHOR");
    gui::same_line();
    gui::item_size({ INPUT_WIDTH, 16 });
    gui::input_text(song.authorname);

    gui::item_size({ app::CANVAS_WIDTH - INPUT_WIDTH, 16 });
    gui::text("COPYR.");
    gui::same_line();
    gui::item_size({ INPUT_WIDTH, 16 });
    gui::input_text(song.copyrightname);



    gui::item_size(16);
    gui::item_box();



    draw_load_window();


}

} // namespace project_view
