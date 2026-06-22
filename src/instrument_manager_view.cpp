#include "instrument_manager_view.hpp"
#include "instrument_view.hpp"
#include "app.hpp"
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
std::array<char, 32>     g_file_name;
std::vector<std::string> g_preset_names;
std::vector<std::string> g_user_names;
int                      g_file_scroll;
int                      g_preset_scroll;


#define FILE_SUFFIX ".ins"

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
        app::alert("LOAD ERROR: bad file format");
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
    return true;
}

void load_preset() {
    std::vector<uint8_t> buffer;
    if (!platform::load_asset("instruments/" + std::string(g_file_name.data()) + FILE_SUFFIX, buffer)) {
        app::alert("LOAD ERROR: cannot open file");
        return;
    }
    std::istringstream stream(std::string(buffer.begin(), buffer.end()), std::ios::binary);
    load_instrument(stream);
}

void load_user() {
    std::ifstream stream(g_instruments_dir + g_file_name.data() + FILE_SUFFIX, std::ios::binary);
    if (!stream.is_open()) {
        app::alert("LOAD ERROR: cannot open file");
        return;
    }
    load_instrument(stream);
}

void save() {
    std::ofstream stream(g_instruments_dir + g_file_name.data() + FILE_SUFFIX, std::ios::binary);
    if (!stream.is_open()) {
        app::alert("SAVE ERROR");
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
    init();
}

} // namespace


void reset() {
    g_instruments_dir = {};
    g_tab             = Tab::Files;
    g_file_name       = {};
    g_preset_names    = {};
    g_user_names      = {};
    g_file_scroll     = {};
    g_preset_scroll   = {};
}


void init() {
    if (g_instruments_dir.empty()) {
        g_instruments_dir = app::storage_dir() + "/instruments/";
        fs::create_directories(g_instruments_dir);
    }

    g_preset_names.clear();
    for (std::string const& s : platform::list_assets("instruments")) {
        auto path = fs::path(s);
        if (path.extension() != FILE_SUFFIX) continue;
        g_preset_names.emplace_back(path.stem().string());
    }
    std::sort(g_preset_names.begin(), g_preset_names.end(), [](std::string const& a, std::string const& b) {
        return strcasecmp(a.c_str(), b.c_str()) < 0;
    });

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


void draw() {

    gui::align(gui::Align::Center);
    gui::button_style(gui::ButtonStyle::Tab);
    gui::item_size({ app::CANVAS_WIDTH / 2, app::BUTTON_HEIGHT });
    if (gui::button("FILES", g_tab == Tab::Files)) g_tab = Tab::Files;
    gui::same_line();
    if (gui::button("PRESETS", g_tab == Tab::Presets)) g_tab = Tab::Presets;

    gui::button_style(gui::ButtonStyle::Normal);
    gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
    gui::separator();
    gui::align(gui::Align::Left);
    if (g_tab == Tab::Files) gui::input_text(g_file_name);

    ivec2 list_cursor = gui::cursor();
    int toolbar_rows = 1;
    int list_space = app::canvas_height() - list_cursor.y - piano::HEIGHT;
    list_space -= gui::FRAME_WIDTH + app::BUTTON_HEIGHT * toolbar_rows;

    int list_page = std::max(1, (list_space - 2) / app::MAX_ROW_HEIGHT);

    int& scroll    = g_tab == Tab::Presets ? g_preset_scroll : g_file_scroll;
    int  list_size = g_tab == Tab::Presets ? int(g_preset_names.size()) : int(g_user_names.size());
    auto names     = g_tab == Tab::Presets ? g_preset_names : g_user_names;

    gui::cursor(list_cursor + ivec2(0, 1));
    gui::item_size({ app::CANVAS_WIDTH - app::SCROLL_WIDTH, app::MAX_ROW_HEIGHT });
    gui::button_style(gui::ButtonStyle::TableCell);
    gui::align(gui::Align::Left);
    for (int i = 0; i < list_page; ++i) {
        int row = scroll + i;
        if (row >= list_size) {
            gui::item_box();
            continue;
        }
        char const* s = names[row].c_str();
        if (gui::button(s, strcmp(s, g_file_name.data()) == 0)) {
            snprintf(g_file_name.data(), g_file_name.size(), "%s", s);
        }
    }
    gui::align(gui::Align::Center);
    gui::button_style(gui::ButtonStyle::Normal);

    gui::cursor({ app::CANVAS_WIDTH - app::SCROLL_WIDTH, list_cursor.y });
    gui::item_size({ app::SCROLL_WIDTH, list_page * app::MAX_ROW_HEIGHT + 2 });
    int max_scroll = std::max(0, list_size - list_page);
    scroll = std::min(scroll, max_scroll);
    gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
    gui::vertical_drag_bar(scroll, 0, max_scroll, list_page);

    gui::cursor({ 0, gui::cursor().y });
    gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
    gui::separator();

    if (g_tab == Tab::Presets) {
        bool selected = false;
        for (std::string const& name : g_preset_names) {
            if (strcmp(name.c_str(), g_file_name.data()) == 0) {
                selected = true;
                break;
            }
        }

        gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
        gui::disabled(!selected);
        if (gui::button("LOAD")) load_preset();
        gui::disabled(false);
    }
    else {
        bool selected = false;
        for (std::string const& name : g_user_names) {
            if (strcmp(name.c_str(), g_file_name.data()) == 0) {
                selected = true;
                break;
            }
        }

        gui::item_size({ app::CANVAS_WIDTH / 3, app::BUTTON_HEIGHT });
        gui::disabled(!selected);
        if (gui::button("LOAD")) load_user();
        gui::same_line();
        gui::disabled(g_file_name[0] == '\0');
        if (gui::button("SAVE")) {
            if (selected) {
                app::confirm("OVERWRITE THE EXISTING INSTRUMENT?", [](bool ok) {
                    if (ok) save();
                });
            }
            else {
                save();
            }
        }
        gui::same_line();
        gui::disabled(!selected);
        if (gui::button("DELETE")) {
            app::confirm("DELETE INSTRUMENT?", [](bool ok) {
                if (!ok) return;
                fs::remove(g_instruments_dir + g_file_name.data() + FILE_SUFFIX);
                init();
            });
        }
        gui::disabled(false);
    }

    app::draw_confirm();
    piano::draw();
}


} // namespace instrument_manager_view
