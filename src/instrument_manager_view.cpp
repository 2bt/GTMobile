#include "instrument_manager_view.hpp"
#include "instrument_view.hpp"
#include "app.hpp"
#include "log.hpp"
#include "platform.hpp"
#include "piano.hpp"
#include <filesystem>
#include <cstring>
#include <fstream>


namespace fs = std::filesystem;


namespace instrument_manager_view {
namespace {

gt::Song&                g_song = app::song();
std::string              g_instruments_dir = {};
std::array<char, 32>     g_file_name       = {};
std::vector<std::string> g_file_names      = {};
int                      g_file_scroll     = 0;
std::string              g_status_msg      = {};
float                    g_status_age      = 0.0f;


#define FILE_SUFFIX ".ins"


void status(std::string const& msg) {
    g_status_msg = msg;
    g_status_age = 0.0f;
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

void load() {
    std::ifstream stream(g_instruments_dir + g_file_name.data() + FILE_SUFFIX, std::ios::binary);
    if (!stream.is_open()) {
        status("LOAD ERROR: cannot open file");
        return;
    }
    std::array<char, 4> ident;
    read(stream, ident);
    if (ident != std::array<char, 4>{'G', 'T', 'I', '5'}) {
        status("LOAD ERROR: bad file format");
        return;
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


} // namespace


void reset() {
    g_instruments_dir = {};
    g_file_name       = {};
    g_file_names      = {};
    g_file_scroll     = 0;
    g_status_msg      = {};
    g_status_age      = 0.0f;
}


void init() {
    if (g_instruments_dir.empty()) {
        g_instruments_dir = app::storage_dir() + "/instruments/";
        // copy instruments
        fs::create_directories(g_instruments_dir);
        for (std::string const& s : platform::list_assets("instruments")) {
            std::string dst_name = g_instruments_dir + s;
            if (fs::exists(dst_name)) continue;
            std::vector<uint8_t> buffer;
            if (!platform::load_asset("instruments/" + s, buffer)) continue;
            std::ofstream f(dst_name, std::ios::binary);
            f.write((char const*)buffer.data(), buffer.size());
            f.close();
        }
    }

    g_file_names.clear();
    for (auto const& entry : fs::directory_iterator(g_instruments_dir)) {
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
        C1 = 12 + 8 * 8,
        C2 = app::CANVAS_WIDTH - C1,
    };

    gui::align(gui::Align::Left);

    gui::item_size({ C1, app::BUTTON_HEIGHT });
    gui::item_box();
    gui::same_line();
    gui::item_size({ C2, app::BUTTON_HEIGHT });
    gui::input_text(g_file_name);
    gui::align(gui::Align::Center);

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
        load();
    }
    gui::disabled(g_file_name[0] == '\0');
    if (gui::button("SAVE")) {
        if (file_selected) {
            app::confirm("OVERWRITE THE EXISTING INSTRUMENT?", [](bool ok) {
                if (ok) save();
            });
        }
        else {
            save();
        }
    }
    gui::disabled(!file_selected);
    if (gui::button("DELETE")) {
        app::confirm("DELETE INSTRUMENT?", [](bool ok) {
            if (!ok) return;
            fs::remove(g_instruments_dir + g_file_name.data() + FILE_SUFFIX);
            init();
            status("INSTRUMENT WAS DELETED");
        });
    }
    gui::disabled(false);

    app::draw_confirm();
    piano::draw();
}


} // namespace instrument_manager_view
