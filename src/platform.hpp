#pragma once
#include <string>
#include <vector>
#include <cstdint>


namespace platform {
    bool load_asset(std::string const& name, std::vector<uint8_t>& buf);
    std::vector<std::string> list_assets(std::string const& dir);
    void show_keyboard(bool enabled);
    void export_file(std::string const& path, std::string const& title, bool delete_when_done);
    void start_song_import();
    void update_setting(int i);
}
