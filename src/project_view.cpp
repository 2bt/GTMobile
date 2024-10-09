#include "project_view.hpp"
#include "gui.hpp"
#include "app.hpp"
#include "log.hpp"
#include "platform.hpp"
#include "piano.hpp"
#include "song_view.hpp"
#include <array>
#include <filesystem>
#include <cstring>
#include <fstream>


namespace fs = std::filesystem;


namespace project_view {
namespace {


using ConfirmCallback = void(*)(bool);
ConfirmCallback g_confirm_callback;
std::string     g_confirm_msg;
void draw_confirm() {
    if (g_confirm_msg.empty()) return;
    gui::Box box = gui::begin_window({ app::CANVAS_WIDTH - 24 * 2, app::BUTTON_HEIGHT * 2 });

    gui::item_size({ box.size.x, app::BUTTON_HEIGHT });
    gui::align(gui::Align::Center);
    gui::text(g_confirm_msg.c_str());
    if (!g_confirm_callback) {
        if (gui::button("OK")) g_confirm_msg.clear();
        return;
    }
    gui::item_size({ box.size.x / 2, app::BUTTON_HEIGHT });
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



enum class Dialog {
    none,
    load,
    save,
};

Dialog                   g_dialog = Dialog::none;
std::string              g_songs_dir;
std::array<char, 32>     g_file_name;
std::vector<std::string> g_file_names;
int                      g_file_scroll = 0;
std::string              g_status_msg;
float                    g_status_age = 0.0f;


#define FILE_SUFFIX ".sng"


void status(std::string const& msg) {
    g_status_msg = msg;
    g_status_msg = msg.substr(0, 20);
    if (msg.size() > 20) g_status_msg += "\n" + msg.substr(20);
    g_status_age = 0.0f;
}


bool ends_with(std::string const& str, char const* suffix) {
    size_t l = std::strlen(suffix);
    if (l > str.size()) {
        return false;
    }
    return std::equal(suffix, suffix + l, str.end() - l);
}


void draw_load_window() {
    int table_height = app::canvas_height() - app::BUTTON_HEIGHT * 2 - 30;
    int page = table_height / app::MAX_ROW_HEIGHT;
    table_height = page * app::MAX_ROW_HEIGHT;
    gui::Box box = gui::begin_window({
        app::CANVAS_WIDTH - 30,
        table_height + app::BUTTON_HEIGHT * 2,
    });

    gui::item_size({ box.size.x, app::BUTTON_HEIGHT });
    gui::align(gui::Align::Center);
    gui::text("LOAD SONG");

    gui::align(gui::Align::Left);
    gui::item_size({ box.size.x - app::SCROLL_WIDTH, app::MAX_ROW_HEIGHT });
    bool file_selected = false;

    for (int i = 0; i < page; ++i) {
        size_t row = g_file_scroll + i;
        if (row >= g_file_names.size()) {
            gui::item_box();
            continue;
        }
        char const* s = g_file_names[row].c_str();
        bool selected = strcmp(s, g_file_name.data()) == 0;
        file_selected |= selected;
        if (gui::button(s, selected)) {
            strncpy(g_file_name.data(), s, g_file_name.size() - 1);
            file_selected = true;
        }
    }

    gui::align(gui::Align::Center);
    gui::item_size({ box.size.x / 2, app::BUTTON_HEIGHT });
    if (gui::button("CANCEL")) g_dialog = Dialog::none;
    gui::same_line();

    gui::disabled(!file_selected);
    if (gui::button("LOAD")) {
        app::player().stop_song();
        song_view::reset();
        bool ok = app::song().load((g_songs_dir + g_file_name.data() + FILE_SUFFIX).c_str());
        if (ok) status("SONG WAS LOADED");
        else status("LOAD ERROR");
        g_dialog = Dialog::none;
    }
    gui::disabled(false);

    // scrollbar
    gui::cursor(box.pos + ivec2(box.size.x - app::SCROLL_WIDTH, app::BUTTON_HEIGHT));
    gui::item_size({ app::SCROLL_WIDTH, table_height });
    int max_scroll = std::max(0, int(g_file_names.size()) - page);
    gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
    gui::vertical_drag_bar(g_file_scroll, 0, max_scroll, page);

    gui::end_window();
}

void save() {
    bool ok = app::song().save((g_songs_dir + g_file_name.data() + FILE_SUFFIX).c_str());
    init();
    if (ok) status("SONG WAS SAVED");
    else status("SAVE ERROR");
    g_dialog = Dialog::none;
}

void draw_save_window() {
    int table_height = app::canvas_height() - app::BUTTON_HEIGHT * 3 - 30;
    int page = table_height / app::MAX_ROW_HEIGHT;
    table_height = page * app::MAX_ROW_HEIGHT;
    gui::Box box = gui::begin_window({
        app::CANVAS_WIDTH - 30,
        table_height + app::BUTTON_HEIGHT * 3,
    });

    gui::item_size({ box.size.x, app::BUTTON_HEIGHT });
    gui::align(gui::Align::Center);
    gui::text("SAVE SONG");

    gui::align(gui::Align::Left);
    gui::item_size({ box.size.x - app::SCROLL_WIDTH, app::MAX_ROW_HEIGHT });
    bool file_selected = false;

    for (int i = 0; i < page; ++i) {
        size_t row = g_file_scroll + i;
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


    gui::item_size({ box.size.x, app::BUTTON_HEIGHT });
    gui::input_text(g_file_name);

    gui::align(gui::Align::Center);
    gui::item_size({ box.size.x / 2, app::BUTTON_HEIGHT });
    if (gui::button("CANCEL")) g_dialog = Dialog::none;
    gui::same_line();

    gui::disabled(g_file_name[0] == '\0');
    if (gui::button("SAVE")) {
        if (file_selected) {
            confirm("OVERWRITE THE EXISTING SONG?", [](bool ok) {
                if (ok) save();
            });
        }
        else {
            save();
        }
    }
    gui::disabled(false);

    // scrollbar
    gui::cursor(box.pos + ivec2(box.size.x - app::SCROLL_WIDTH, app::BUTTON_HEIGHT));
    gui::item_size({ app::SCROLL_WIDTH, table_height });
    int max_scroll = std::max(0, int(g_file_names.size()) - page);
    gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
    gui::vertical_drag_bar(g_file_scroll, 0, max_scroll, page);

    draw_confirm();
    gui::end_window();
}


} // namespace


void set_storage_dir(std::string const& storage_dir) {
    g_songs_dir = storage_dir + "/songs/";
}


void init() {
    static bool init_dirs_done = false;
    if (!init_dirs_done) {
        init_dirs_done = true;
        if (g_songs_dir.empty()) {
            set_storage_dir(".");
        }

        // copy demo songs
        fs::create_directories(g_songs_dir);
        for (std::string const& s : platform::list_assets("songs")) {
            if (!ends_with(s, FILE_SUFFIX)) continue;
            std::string dst_name = g_songs_dir + s;
            if (fs::exists(dst_name)) continue;
            std::vector<uint8_t> buffer;
            if (!platform::load_asset("songs/" + s, buffer)) continue;
            std::ofstream f(dst_name, std::ios::binary);
            f.write((char const*)buffer.data(), buffer.size());
            f.close();
        }
    }

    g_file_names.clear();
    for (auto const& entry : fs::directory_iterator(g_songs_dir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension().string() != FILE_SUFFIX) continue;
        g_file_names.emplace_back(entry.path().stem().string());
    }
    std::sort(g_file_names.begin(), g_file_names.end());
    g_status_msg = "";
}


void draw() {

    enum {
        INPUT_WIDTH = 32 * 8 + 12,
    };


    gt::Song& song = app::song();

    gui::align(gui::Align::Left);
    gui::item_size({ app::CANVAS_WIDTH - INPUT_WIDTH, app::BUTTON_HEIGHT });
    gui::text("TITLE");
    gui::same_line();
    gui::item_size({ INPUT_WIDTH, app::BUTTON_HEIGHT });
    gui::input_text(song.song_name);

    gui::item_size({ app::CANVAS_WIDTH - INPUT_WIDTH, app::BUTTON_HEIGHT });
    gui::text("AUTHOR");
    gui::same_line();
    gui::item_size({ INPUT_WIDTH, app::BUTTON_HEIGHT });
    gui::input_text(song.author_name);

    gui::item_size({ app::CANVAS_WIDTH - INPUT_WIDTH, app::BUTTON_HEIGHT });
    gui::text("RELEASED");
    gui::same_line();
    gui::item_size({ INPUT_WIDTH, app::BUTTON_HEIGHT });
    gui::input_text(song.copyright_name);


    gui::align(gui::Align::Center);
    gui::item_size({ app::CANVAS_WIDTH / 3, app::BUTTON_HEIGHT });
    if (gui::button("RESET")) {
        confirm("LOSE CHANGES TO THE CURRENT SONG?", [](bool ok) {
            if (ok) {
                app::player().stop_song();
                app::song().clear();
                song_view::reset();
                status("SONG WAS RESET");
            }
        });
    }
    gui::same_line();
    if (gui::button("LOAD")) {
        g_dialog = Dialog::load;
    }
    gui::same_line();
    if (gui::button("SAVE")) {
        g_dialog = Dialog::save;
    }

    gui::align(gui::Align::Left);
    gui::text("%s", g_status_msg.c_str());
    g_status_age += gui::get_frame_time();
    if (g_status_age > 2.0f) {
        g_status_msg = "";
    }

    if (g_dialog == Dialog::load) draw_load_window();
    else if (g_dialog == Dialog::save) draw_save_window();
    else draw_confirm();

    piano::draw();
}

} // namespace project_view
