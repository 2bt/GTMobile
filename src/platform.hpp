#pragma once
#include <string>
#include <vector>
#include <cstdint>


namespace platform {
    bool load_asset(char const* name, std::vector<uint8_t>& buf);
    bool list_assets(char const* dir, std::vector<std::string>& list);
    void show_keyboard(bool enabled);
}
