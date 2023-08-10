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


using ConfirmCallback = void(*)(bool);
ConfirmCallback g_confirm_callback;
std::string     g_confirm_msg;
void draw_confirm() {
    if (g_confirm_msg.empty()) return;
    gui::Box box = gui::begin_window({ app::CANVAS_WIDTH - 24 * 2, app::BUTTON_WIDTH * 2 });

    gui::item_size({ box.size.x, app::BUTTON_WIDTH });
    gui::align(gui::Align::Center);
    gui::text(g_confirm_msg.c_str());
    if (!g_confirm_callback) {
        if (gui::button("OK")) g_confirm_msg.clear();
        return;
    }
    gui::item_size({ box.size.x / 2, app::BUTTON_WIDTH });
    if (gui::button("CANCEL")) {
        g_confirm_msg.clear();
        g_confirm_callback(false);
    }
    gui::same_line();
    if (gui::button("OK")) {
        g_confirm_msg.clear();
        g_confirm_callback(true);
    }
    gui::end_window();
}
void confirm(std::string msg, ConfirmCallback cb) {
    g_confirm_msg      = std::move(msg);
    g_confirm_callback = cb;
}



enum Dialog {
    None,
    Load,
};

Dialog                   g_dialog = Dialog::None;
std::array<char, 32>     g_file_name;
std::vector<std::string> g_file_names;



void draw_load_window() {
    if (g_dialog != Dialog::Load) return;

    enum {
        PAGE = 20,
    };
    static int scroll = 0;

    gui::Box box = gui::begin_window({ app::CANVAS_WIDTH - 24 * 2, PAGE * 16 + app::BUTTON_WIDTH * 2  });

    gui::item_size({ box.size.x, app::BUTTON_WIDTH });
    gui::align(gui::Align::Center);
    gui::text("LOAD SONG");

    gui::align(gui::Align::Left);
    gui::item_size({ box.size.x - app::BUTTON_WIDTH, 16 });
    bool file_selected = false;

    for (int i = 0; i < PAGE; ++i) {
        size_t row = scroll + i;
        if (row >= g_file_names.size()) {
            gui::item_box();
            continue;
        }
        char const* s = g_file_names[row].c_str();

        bool selected = strcmp(s, g_file_name.data()) == 0;
        file_selected |= selected;
        if (gui::button(s, selected)) {
            strncpy(g_file_name.data(), s, g_file_name.size() - 1);
        }
    }

    gui::align(gui::Align::Center);
    gui::item_size({ box.size.x / 2, app::BUTTON_WIDTH });
    if (gui::button("CANCEL")) g_dialog = Dialog::None;
    gui::same_line();
    if (gui::button("LOAD") && file_selected) {
        app::player().init_song(0, gt::Player::PLAY_STOP);
        std::vector<uint8_t> buffer;
        if (platform::load_asset((std::string("songs/") + g_file_name.data()).c_str(), buffer)) {
            app::song().load(buffer.data(), buffer.size());
            g_dialog = Dialog::None;
        }
    }

    // scrollbar
    gui::cursor(box.pos + ivec2(box.size.x - app::BUTTON_WIDTH, app::BUTTON_WIDTH));
    gui::item_size({ app::BUTTON_WIDTH, PAGE * 16 });
    int max_scroll = std::max(0, int(g_file_names.size()) - PAGE);
    gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
    gui::vertical_drag_bar(scroll, 0, max_scroll, PAGE);

    gui::end_window();
}



} // namespace


void init() {


}


void draw() {

    enum {
        INPUT_WIDTH = 32 * 8 + 12,
    };


    gt::Song& song = app::song();

    gui::item_size(app::BUTTON_WIDTH);
    gui::item_box();

    gui::align(gui::Align::Left);
    gui::item_size({ app::CANVAS_WIDTH - INPUT_WIDTH, app::BUTTON_WIDTH });
    gui::text("TITLE");
    gui::same_line();
    gui::item_size({ INPUT_WIDTH, app::BUTTON_WIDTH });
    gui::input_text(song.songname);

    gui::item_size({ app::CANVAS_WIDTH - INPUT_WIDTH, app::BUTTON_WIDTH });
    gui::text("AUTHOR");
    gui::same_line();
    gui::item_size({ INPUT_WIDTH, app::BUTTON_WIDTH });
    gui::input_text(song.authorname);

    gui::item_size({ app::CANVAS_WIDTH - INPUT_WIDTH, app::BUTTON_WIDTH });
    gui::text("RELEASED");
    gui::same_line();
    gui::item_size({ INPUT_WIDTH, app::BUTTON_WIDTH });
    gui::input_text(song.copyrightname);


    gui::item_size(app::BUTTON_WIDTH);
    gui::item_box();

    gui::align(gui::Align::Center);
    gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_WIDTH });
    if (gui::button("RESET")) {
        app::player().init_song(0, gt::Player::PLAY_STOP);
        confirm("LOSE CHANGES TO THE CURRENT SONG?", [](bool ok) {
            if (ok) app::song().clear();
        });
    }
    if (gui::button("LOAD")) {
        g_dialog = Dialog::Load;

        app::player().init_song(0, gt::Player::PLAY_STOP);
        platform::list_assets("songs", g_file_names);
    }
    if (gui::button("SAVE")) {

    }


    draw_load_window();
    draw_confirm();
}

} // namespace project_view
