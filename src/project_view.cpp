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
    if (gui::button("OK")) {
        g_confirm_msg.clear();
        g_confirm_callback(true);
    }
    gui::same_line();
    if (gui::button("CANCEL")) {
        g_confirm_msg.clear();
        g_confirm_callback(false);
    }
    gui::end_window();
}
void confirm(std::string msg, ConfirmCallback cb) {
    g_confirm_msg      = std::move(msg);
    g_confirm_callback = cb;
}



gt::Song&                g_song = app::song();
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


void save() {
    bool ok = g_song.save((g_songs_dir + g_file_name.data() + FILE_SUFFIX).c_str());
    init();
    if (ok) status("SONG WAS SAVED");
    else status("SAVE ERROR");
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
    std::sort(g_file_names.begin(), g_file_names.end(), [](std::string const& a, std::string const& b) {
        return strcasecmp(a.c_str(), b.c_str()) < 0;
    });
    g_status_msg = "";
}


void draw() {

    enum {
        C2 = 32 * 8 + 12,
        C1 = app::CANVAS_WIDTH - C2,
    };



    gui::item_size({ C1, app::BUTTON_HEIGHT });
    gui::text("TITLE");
    gui::same_line();
    gui::item_size({ C2, app::BUTTON_HEIGHT });
    gui::input_text(g_song.song_name);

    gui::item_size({ C1, app::BUTTON_HEIGHT });
    gui::text("AUTHOR");
    gui::same_line();
    gui::item_size({ C2, app::BUTTON_HEIGHT });
    gui::input_text(g_song.author_name);

    gui::item_size({ C1, app::BUTTON_HEIGHT });
    gui::text("RELEASED");
    gui::same_line();
    gui::item_size({ C2, app::BUTTON_HEIGHT });
    gui::input_text(g_song.copyright_name);

    gui::item_size({ C1, app::BUTTON_HEIGHT });
    gui::text("FILE");
    gui::same_line();
    gui::item_size({ C2, app::BUTTON_HEIGHT });
    gui::input_text(g_file_name);

    gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
    gui::separator();

    ivec2 cursor = gui::cursor();
    int table_height = app::canvas_height() - piano::HEIGHT - cursor.y - 2 - app::MAX_ROW_HEIGHT - gui::FRAME_WIDTH;
    int page = table_height / app::MAX_ROW_HEIGHT;
    table_height = page * app::MAX_ROW_HEIGHT + 2;

    gui::item_size({ C1 - gui::FRAME_WIDTH, table_height });
    gui::item_box();
    gui::same_line();
    gui::separator();
    ivec2 table_cursor = gui::cursor();
    gui::same_line(false);
    gui::item_size(app::CANVAS_WIDTH);
    gui::separator();
    gui::item_size({ app::CANVAS_WIDTH, app::canvas_height() - gui::cursor().y - piano::HEIGHT });
    gui::align(gui::Align::Left);
    gui::text("%s", g_status_msg.c_str());
    g_status_age += gui::get_frame_time();
    if (g_status_age > 2.0f) {
        g_status_msg = "";
    }


    // file table
    gui::cursor(table_cursor + ivec2(0, 1));
    gui::item_size({ C2 - app::SCROLL_WIDTH, app::MAX_ROW_HEIGHT });
    gui::button_style(gui::ButtonStyle::TableCell);
    gui::align(gui::Align::Left);
    for (int i = 0; i < page; ++i) {
        size_t row = g_file_scroll + i;
        if (row >= g_file_names.size()) {
            gui::item_box();
            continue;
        }
        char const* s = g_file_names[row].c_str();
        bool selected = strcmp(s, g_file_name.data()) == 0;
        if (gui::button(s, selected)) {
            strncpy(g_file_name.data(), s, g_file_name.size() - 1);
        }
    }
    gui::align(gui::Align::Center);
    gui::button_style(gui::ButtonStyle::Normal);
    gui::cursor(ivec2(app::CANVAS_WIDTH - app::SCROLL_WIDTH, cursor.y));
    gui::item_size({ app::SCROLL_WIDTH, table_height });
    int max_scroll = std::max(0, int(g_file_names.size()) - page);
    gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
    gui::vertical_drag_bar(g_file_scroll, 0, max_scroll, page);


    // button column
    gui::cursor(cursor);
    gui::align(gui::Align::Center);
    gui::item_size({ C1 - gui::FRAME_WIDTH, app::BUTTON_HEIGHT });
    bool file_selected = false;
    for (std::string const& n : g_file_names) {
        if (strcmp(n.c_str(), g_file_name.data()) == 0) {
            file_selected = true;
            break;
        }
    }
    gui::disabled(!file_selected);
    if (gui::button("LOAD")) {
        confirm("LOSE CHANGES TO THE CURRENT SONG?", [](bool ok) {
            if (!ok) return;
            app::player().stop_song();
            app::player().reset();
            song_view::reset();
            try {
                g_song.load((g_songs_dir + g_file_name.data() + FILE_SUFFIX).c_str());
                status("SONG WAS LOADED");
            }
            catch (gt::LoadError const& e) {
                g_song.clear();
                status("LOAD ERROR: " + e.msg);
            }
        });
    }
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
    gui::disabled(!file_selected);
    if (gui::button("DELETE")) {
        confirm("DELETE SONG?", [](bool ok) {
            if (!ok) return;
            fs::remove(g_songs_dir + g_file_name.data() + FILE_SUFFIX);
            init();
            status("SONG WAS DELETED");
        });
    }
    gui::disabled(false);
    if (gui::button("RESET")) {
        confirm("LOSE CHANGES TO THE CURRENT SONG?", [](bool ok) {
            if (!ok) return;
            app::player().stop_song();
            app::player().reset();
            g_song.clear();
            song_view::reset();
            status("SONG WAS RESET");
        });
    }


    draw_confirm();

    piano::draw();
}

} // namespace project_view
