#pragma once
#include <string>
#include <vector>
#include <cstdint>


namespace platform {
    bool load_asset(std::string const& name, std::vector<uint8_t>& buf);
    std::vector<std::string> list_assets(std::string const& dir);
    void show_keyboard(bool enabled);
    void export_song(std::string const& path, std::string const& title);
    void start_song_import();
    void update_setting(int i);
    bool poll_midi_event(uint8_t& status, uint8_t& data1, uint8_t& data2);
}
