#include "instrument_manager_view.hpp"
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
std::string              g_instruments_dir;
std::array<char, 32>     g_file_name;
std::vector<std::string> g_file_names;
int                      g_file_scroll = 0;
std::string              g_status_msg;
float                    g_status_age = 0.0f;


#define FILE_SUFFIX ".ins"


} // namespace


void init() {
    static bool init_dirs_done = false;
    if (!init_dirs_done) {
        init_dirs_done = true;
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


    piano::draw();
}


} // namespace instrument_manager_view
