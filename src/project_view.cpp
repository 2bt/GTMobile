#include "project_view.hpp"
#include "app.hpp"
#include "log.hpp"
#include "platform.hpp"
#include "piano.hpp"
#include "song_view.hpp"
#include <filesystem>
#include <cstring>
#include <fstream>
#include <thread>
#include <cassert>
#include <sndfile.h>


namespace fs = std::filesystem;


namespace project_view {
namespace {

enum class ExportFormat { Wav, Ogg };

gt::Song&                g_song = app::song();
std::string              g_song_dir;
std::array<char, 32>     g_file_name;
std::vector<std::string> g_file_names;
int                      g_file_scroll;
std::string              g_status_msg;
float                    g_status_age;

bool                     g_show_export_window;
ExportFormat             g_export_format;
std::thread              g_export_thread;
bool                     g_export_canceled;
bool                     g_export_done;
float                    g_export_progress;


#define FILE_SUFFIX ".sng"


void status(std::string const& msg) {
    g_status_msg = msg;
    g_status_age = 0.0f;
}


void save() {
    bool ok = g_song.save((g_song_dir + g_file_name.data() + FILE_SUFFIX).c_str());
    init();
    if (ok) status("SONG WAS SAVED");
    else status("SAVE ERROR");
}




void start_export() {
    assert(g_file_name.data()[0] != '\0');

    std::string export_dir = app::storage_dir() + "/exports/";
    fs::create_directories(export_dir);

    SF_INFO info = { 0, app::MIXRATE, 1 };
    std::string path = export_dir + std::string(g_file_name.data());
    if (g_export_format == ExportFormat::Ogg) {
        info.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
        path += ".ogg";
    }
    else {
        info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        path += ".wav";
    }

    SNDFILE* sndfile = sf_open(path.c_str(), SFM_WRITE, &info);
    if (!sndfile) {
        status("EXPORT ERROR");
        g_show_export_window = false;
        return;
    }

    sf_set_string(sndfile, SF_STR_TITLE, g_song.song_name.data());
    sf_set_string(sndfile, SF_STR_ARTIST, g_song.author_name.data());

    g_export_canceled = false;
    g_export_done     = false;
    g_export_progress = 0.0f;
    g_export_thread = std::thread([sndfile] {
        std::array<int16_t, 4096> buffer;

        int samples;
        {
            // calculate song length in samples
            gt::Player player{ g_song };
            player.play_song();
            int tick_count = 1;
            while (player.channel_loop_counter(0) == 0) {
                player.play_routine();
                ++tick_count;
            }
            tick_count += player.channel_tempo(0);
            int const ticks_per_second = g_song.multiplier * 50 ?: 25;
            int const cycles_per_tick  = Sid::CLOCKRATE_PAL / ticks_per_second;
            uint64_t cycles = cycles_per_tick * tick_count;
            samples = cycles * app::MIXRATE / Sid::CLOCKRATE_PAL;
        }

        gt::Player player{ g_song };
        for (int i = 0; i < 3; ++i) {
            player.set_channel_active(i, app::player().is_channel_active(i));
        }
        player.play_song();
        Sid sid;
        sid.init(Sid::Model(g_song.model), Sid::SamplingMethod::ResampleInterpolate);
        app::Mixer mixer{ player, sid };

        for(int samples_left = samples; samples_left > 0 && !g_export_canceled;) {
            int len = std::min<int>(samples_left, buffer.size());
            samples_left -= len;
            mixer.mix(buffer.data(), len);
            sf_writef_short(sndfile, buffer.data(), len);
            g_export_progress = float(samples - samples_left) / samples;
        }

        sf_close(sndfile);
        g_export_done = true;
    });
}

} // namespace


void reset() {
    g_song_dir           = {};
    g_file_name          = {};
    g_file_names         = {};
    g_file_scroll        = {};
    g_status_msg         = {};
    g_status_age         = {};
    g_show_export_window = {};
    g_export_format      = {};
}

void init() {
    if (g_song_dir.empty()) {
        g_song_dir = app::storage_dir() + "/songs/";
        // copy demo songs
        fs::create_directories(g_song_dir);
        for (std::string const& s : platform::list_assets("songs")) {
            std::string dst_name = g_song_dir + s;
            if (fs::exists(dst_name)) continue;
            std::vector<uint8_t> buffer;
            if (!platform::load_asset("songs/" + s, buffer)) continue;
            std::ofstream f(dst_name, std::ios::binary);
            f.write((char const*)buffer.data(), buffer.size());
            f.close();
        }
    }

    g_file_names.clear();
    for (auto const& entry : fs::directory_iterator(g_song_dir)) {
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
    gui::align(gui::Align::Center);

    gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
    gui::separator();

    ivec2 table_cursor = gui::cursor();
    int table_height = app::canvas_height() - piano::HEIGHT - table_cursor.y - 2 - app::MAX_ROW_HEIGHT - gui::FRAME_WIDTH;
    int page = table_height / app::MAX_ROW_HEIGHT;
    table_height = page * app::MAX_ROW_HEIGHT + 2;

    gui::item_size({ C2, table_height });
    gui::item_box();
    gui::same_line();
    gui::separator();
    ivec2 button_cursor = gui::cursor();
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
    gui::cursor(ivec2(C2 - app::SCROLL_WIDTH, table_cursor.y));
    gui::item_size({ app::SCROLL_WIDTH, table_height });
    int max_scroll = std::max(0, int(g_file_names.size()) - page);
    gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
    gui::vertical_drag_bar(g_file_scroll, 0, max_scroll, page);


    // button column
    gui::cursor(button_cursor);
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
        app::confirm("LOSE CHANGES TO THE CURRENT SONG?", [](bool ok) {
            if (!ok) return;
            app::player().reset();
            song_view::reset();
            try {
                g_song.load((g_song_dir + g_file_name.data() + FILE_SUFFIX).c_str());
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
            app::confirm("OVERWRITE THE EXISTING SONG?", [](bool ok) {
                if (ok) save();
            });
        }
        else {
            save();
        }
    }
    gui::disabled(!file_selected);
    if (gui::button("DELETE")) {
        app::confirm("DELETE SONG?", [](bool ok) {
            if (!ok) return;
            fs::remove(g_song_dir + g_file_name.data() + FILE_SUFFIX);
            init();
            status("SONG WAS DELETED");
        });
    }
    gui::disabled(false);
    if (gui::button("RESET")) {
        app::confirm("LOSE CHANGES TO THE CURRENT SONG?", [](bool ok) {
            if (!ok) return;
            app::player().reset();
            g_song.clear();
            song_view::reset();
            status("SONG WAS RESET");
        });
    }

    gui::disabled(g_file_name[0] == '\0');
    if (gui::button("EXPORT")) g_show_export_window = true;
    gui::disabled(false);



    if (g_show_export_window) {
        gui::Box box = gui::begin_window({ app::CANVAS_WIDTH - 48, app::BUTTON_HEIGHT * 3 });
        gui::item_size({ box.size.x, app::BUTTON_HEIGHT });
        gui::text("SONG EXPORT");

        if (!g_export_thread.joinable()) {

            gui::item_size({ box.size.x / 2, app::BUTTON_HEIGHT });
            gui::text("FORMAT");
            gui::same_line();
            gui::item_size({ box.size.x / 4, app::BUTTON_HEIGHT });
            gui::button_style(gui::ButtonStyle::RadioLeft);
            if (gui::button("WAV", g_export_format == ExportFormat::Wav)) {
                g_export_format = ExportFormat::Wav;
            }
            gui::same_line();
            gui::button_style(gui::ButtonStyle::RadioRight);
            if (gui::button("OGG", g_export_format == ExportFormat::Ogg)) {
                g_export_format = ExportFormat::Ogg;
            }
            gui::button_style(gui::ButtonStyle::Normal);

            gui::item_size({ box.size.x / 2, app::BUTTON_HEIGHT });
            if (gui::button("EXPORT")) {
                start_export();
            }
            gui::same_line();
            if (gui::button("CLOSE")) g_show_export_window = false;
        }
        else {
            gui::item_size({ box.size.x, app::BUTTON_HEIGHT });

            gui::DrawContext& dc = gui::draw_context();
            gui::Box b = gui::item_box();
            b.pos.x  += 1;
            b.pos.y  += 1;
            b.size.x -= 2;
            b.size.y -= 2;
            dc.rgb(color::FRAME);
            dc.fill(b);
            dc.rgb(color::BUTTON_ACTIVE);
            b.pos.x  += 2;
            b.pos.y  += 2;
            b.size.x -= 4;
            b.size.y -= 4;
            b.size.x *= g_export_progress;
            dc.fill(b);
            if (gui::button("CANCEL")) g_export_canceled = true;

            if (g_export_done) {
                g_export_thread.join();
                g_show_export_window = false;
                if (g_export_canceled) {
                    status("SONG EXPORT CANCELED");
                }
                else {
                    status("SONG WAS EXPORTED");
                }
            }
        }
        gui::end_window();
    }

    gui::separator();

    app::draw_confirm();
    piano::draw();
}

} // namespace project_view
