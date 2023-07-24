#pragma once
#include <string>
#include <vector>
#include <cstdint>


namespace platform {
    bool load_asset(std::string const& name, std::vector<uint8_t>& buf);
    void show_keyboard(bool enabled);
}
