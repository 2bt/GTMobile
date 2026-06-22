#include "instrument_manager_view.hpp"
#include "instrument_view.hpp"
#include "app.hpp"
#include "log.hpp"
#include "platform.hpp"
#include "piano.hpp"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <cstring>
#include <fstream>
#include <sstream>


namespace fs = std::filesystem;


namespace instrument_manager_view {
namespace {

enum class Tab { Files, Presets };

gt::Song&                g_song = app::song();
std::string              g_instruments_dir;
Tab                      g_tab = Tab::Files;
std::array<char, 32>     g_preset_name;
std::array<char, 32>     g_file_name;
std::vector<std::string> g_preset_files;
std::vector<std::string> g_user_names;
int                      g_file_scroll;
std::string              g_status_msg;
float                    g_status_age;


#define FILE_SUFFIX ".ins"

void status(std::string const& msg) {
    g_status_msg = msg;
    g_status_age = 0.0f;
}

std::string file_stem(std::string const& path) {
    return fs::path(path).stem().string();
}

template <size_t N>
void copy_name(std::array<char, N>& dst, char const* src) {
    snprintf(dst.data(), dst.size(), "%s", src);
}

template <class T>
bool read(std::istream& stream, T& v) {
    stream.read((char*) &v, sizeof(T));
    return stream.good();
}

uint8_t read8(std::istream& stream) {
    uint8_t b;
    read(stream, b);
    return b;
}

template <class T>
bool write(std::ostream& stream, T const& v) {
    stream.write((char const*) &v, sizeof(T));
    return stream.good();
}

bool load_instrument(std::istream& stream) {
    std::array<char, 4> ident;
    read(stream, ident);
    if (ident != std::array<char, 4>{'G', 'T', 'I', '5'}) {
        status("LOAD ERROR: bad file format");
        return false;
    }
    instrument_view::InstrumentCopyBuffer b = {};
    b.instr_num = 0;
    b.instr.ad = read8(stream);
    b.instr.sr = read8(stream);
    read(stream, b.instr.ptr);
    b.instr.vibdelay = read8(stream);
    b.instr.gatetimer = read8(stream);
    b.instr.firstwave = read8(stream);
    read(stream, b.instr.name);
    for (int t = 0; t < gt::MAX_TABLES; ++t) {
        int len = read8(stream);
        if (len == 0) {
            assert(b.instr.ptr[t] == 0);
            continue;
        }
        assert(b.instr.ptr[t] > 0);
        int s = b.instr.ptr[t] - 1;
        for (int i = 0; i < len; ++i) {
            b.ltable[t][s + i] = read8(stream);
        }
        for (int i = 0; i < len; ++i) {
            b.rtable[t][s + i] = read8(stream);
        }
    }
    b.paste();
    status("INSTRUMENT WAS LOADED");
    return true;
}

void load_preset() {
    std::string asset_file;
    for (std::string const& f : g_preset_files) {
        if (strcmp(file_stem(f).c_str(), g_preset_name.data()) == 0) {
            asset_file = f;
            break;
        }
    }
    if (asset_file.empty()) {
        status("LOAD ERROR: cannot open file");
        return;
    }
    std::vector<uint8_t> buffer;
    if (!platform::load_asset("instruments/" + asset_file, buffer)) {
        status("LOAD ERROR: cannot open file");
        return;
    }
    std::istringstream stream(std::string(buffer.begin(), buffer.end()), std::ios::binary);
    load_instrument(stream);
}

void load_user() {
    std::ifstream stream(g_instruments_dir + g_file_name.data() + FILE_SUFFIX, std::ios::binary);
    if (!stream.is_open()) {
        status("LOAD ERROR: cannot open file");
        return;
    }
    load_instrument(stream);
}

void save() {
    std::ofstream stream(g_instruments_dir + g_file_name.data() + FILE_SUFFIX, std::ios::binary);
    if (!stream.is_open()) {
        status("SAVE ERROR");
        return;
    }
    stream.write("GTI5", 4);
    gt::Instrument const& instr = g_song.instruments[piano::instrument()];
    write(stream, instr.ad);
    write(stream, instr.sr);
    write(stream, instr.ptr);
    write(stream, instr.vibdelay);
    write(stream, instr.gatetimer);
    write(stream, instr.firstwave);
    write(stream, instr.name);
    for (int t = 0; t < gt::MAX_TABLES; t++) {
        if (instr.ptr[t] == 0) {
            write<uint8_t>(stream, 0);
            continue;
        }
        int start = instr.ptr[t] - 1;
        int len = 1;
        if (t != gt::STBL) {
            int i;
            for (i = start; i < gt::MAX_TABLELEN; ++i) {
                if (g_song.ltable[t][i] == 0xff) {
                    int a = g_song.rtable[t][i];
                    assert(a == 0 || (a - 1 >= start && a - 1 < i));
                    ++i;
                    break;
                }
            }
            len = i - start;
        }
        write<uint8_t>(stream, len);
        for (int i = 0; i < len; ++i) {
            write<uint8_t>(stream, g_song.ltable[t][start + i]);
        }
        for (int i = 0; i < len; ++i) {
            write<uint8_t>(stream, g_song.rtable[t][start + i]);
        }
    }
    status("INSTRUMENT WAS SAVED");
    init();
}

void refresh_preset_list() {
    g_preset_files.clear();
    for (std::string const& s : platform::list_assets("instruments")) {
        if (fs::path(s).extension() != FILE_SUFFIX) continue;
        g_preset_files.emplace_back(s);
    }
    std::sort(g_preset_files.begin(), g_preset_files.end(), [](std::string const& a, std::string const& b) {
        return strcasecmp(file_stem(a).c_str(), file_stem(b).c_str()) < 0;
    });
}

void refresh_user_list() {
    g_user_names.clear();
    for (auto const& entry : fs::directory_iterator(g_instruments_dir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension().string() != FILE_SUFFIX) continue;
        g_user_names.emplace_back(entry.path().stem().string());
    }
    std::sort(g_user_names.begin(), g_user_names.end(), [](std::string const& a, std::string const& b) {
        return strcasecmp(a.c_str(), b.c_str()) < 0;
    });
}

size_t list_size() {
    return g_tab == Tab::Presets ? g_preset_files.size() : g_user_names.size();
}

bool row_selected(char const* name) {
    if (g_tab == Tab::Presets) return strcmp(name, g_preset_name.data()) == 0;
    return strcmp(name, g_file_name.data()) == 0;
}

void select_row(char const* name) {
    if (g_tab == Tab::Presets) copy_name(g_preset_name, name);
    else copy_name(g_file_name, name);
}

bool file_selected() {
    if (g_tab == Tab::Presets) return g_preset_name[0] != '\0';
    for (std::string const& n : g_user_names) {
        if (strcmp(n.c_str(), g_file_name.data()) == 0) return true;
    }
    return false;
}


} // namespace


void reset() {
    g_instruments_dir = {};
    g_tab             = Tab::Files;
    g_preset_name     = {};
    g_file_name       = {};
    g_preset_files    = {};
    g_user_names      = {};
    g_file_scroll     = 0;
    g_status_msg      = {};
    g_status_age      = 0.0f;
}


void init() {
    if (g_instruments_dir.empty()) {
        g_instruments_dir = app::storage_dir() + "/instruments/";
        fs::create_directories(g_instruments_dir);
    }

    refresh_preset_list();
    refresh_user_list();
    g_status_msg = "";
}


void draw() {

    gui::button_style(gui::ButtonStyle::Tab);
    gui::item_size({ app::CANVAS_WIDTH / 2, app::BUTTON_HEIGHT });
    if (gui::button("FILES", g_tab == Tab::Files)) {
        if (g_tab != Tab::Files) {
            g_tab = Tab::Files;
            g_file_scroll = 0;
            if (g_preset_name[0] != '\0') {
                copy_name(g_file_name, g_preset_name.data());
            }
        }
    }
    gui::same_line();
    if (gui::button("PRESETS", g_tab == Tab::Presets)) {
        if (g_tab != Tab::Presets) {
            g_tab = Tab::Presets;
            g_file_scroll = 0;
        }
    }
    gui::button_style(gui::ButtonStyle::Normal);
    gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
    gui::separator();

    gui::align(gui::Align::Left);
    gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
    if (g_tab == Tab::Presets) {
        gui::text("%s", g_preset_name.data());
    }
    else {
        gui::input_text(g_file_name);
    }
    gui::align(gui::Align::Center);
    gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
    gui::separator();

    ivec2 table_cursor = gui::cursor();
    int bottom = app::BUTTON_HEIGHT + app::MAX_ROW_HEIGHT + gui::FRAME_WIDTH;
    int table_height = app::canvas_height() - piano::HEIGHT - table_cursor.y - bottom;
    int page = table_height / app::MAX_ROW_HEIGHT;
    table_height = page * app::MAX_ROW_HEIGHT + 2;

    gui::item_size({ app::CANVAS_WIDTH, table_height });
    gui::item_box();

    gui::cursor(table_cursor + ivec2(0, 1));
    gui::item_size({ app::CANVAS_WIDTH - app::SCROLL_WIDTH, app::MAX_ROW_HEIGHT });
    gui::button_style(gui::ButtonStyle::TableCell);
    gui::align(gui::Align::Left);
    for (int i = 0; i < page; ++i) {
        size_t row = g_file_scroll + i;
        if (row >= list_size()) {
            gui::item_box();
            continue;
        }
        std::string preset_stem;
        char const* s;
        if (g_tab == Tab::Presets) {
            preset_stem = file_stem(g_preset_files[row]);
            s = preset_stem.c_str();
        }
        else {
            s = g_user_names[row].c_str();
        }
        if (gui::button(s, row_selected(s))) {
            select_row(s);
        }
    }
    gui::align(gui::Align::Center);
    gui::button_style(gui::ButtonStyle::Normal);
    gui::cursor({ app::CANVAS_WIDTH - app::SCROLL_WIDTH, table_cursor.y });
    gui::item_size({ app::SCROLL_WIDTH, table_height });
    int max_scroll = std::max(0, int(list_size()) - page);
    gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
    gui::vertical_drag_bar(g_file_scroll, 0, max_scroll, page);

    if (g_tab == Tab::Presets) {
        gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
        gui::disabled(g_preset_name[0] == '\0');
        if (gui::button("LOAD")) load_preset();
        gui::disabled(false);
    }
    else {
        gui::item_size({ app::CANVAS_WIDTH / 3, app::BUTTON_HEIGHT });
        gui::disabled(!file_selected());
        if (gui::button("LOAD")) load_user();
        gui::same_line();
        gui::disabled(g_file_name[0] == '\0');
        if (gui::button("SAVE")) {
            if (file_selected()) {
                app::confirm("OVERWRITE THE EXISTING INSTRUMENT?", [](bool ok) {
                    if (ok) save();
                });
            }
            else {
                save();
            }
        }
        gui::same_line();
        gui::disabled(!file_selected());
        if (gui::button("DELETE")) {
            app::confirm("DELETE INSTRUMENT?", [](bool ok) {
                if (!ok) return;
                fs::remove(g_instruments_dir + g_file_name.data() + FILE_SUFFIX);
                init();
                status("INSTRUMENT WAS DELETED");
            });
        }
        gui::disabled(false);
    }

    gui::item_size({ app::CANVAS_WIDTH, app::MAX_ROW_HEIGHT });
    gui::align(gui::Align::Left);
    gui::text("%s", g_status_msg.c_str());
    g_status_age += gui::frame_time();
    if (g_status_age > 2.0f) g_status_msg.clear();

    app::draw_confirm();
    piano::draw();
}


} // namespace instrument_manager_view
