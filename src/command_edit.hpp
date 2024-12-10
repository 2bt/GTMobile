#pragma once

#include <functional>
#include <cstdint>

namespace command_edit {
    enum class Location { Pattern, WaveTable  };
    using CommandCallback = std::function<void(uint8_t, uint8_t)>;
    void reset();
    void init(Location location, uint8_t cmd, uint8_t data, CommandCallback cb);
    void draw();
}
