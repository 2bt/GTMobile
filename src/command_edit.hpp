#pragma once

#include <functional>
#include <cstdint>

namespace command_edit {
    enum class Location { Pattern, WaveTable  };
    void init(Location location, uint8_t cmd, uint8_t data, std::function<void(uint8_t, uint8_t)> on_close);
    void draw();
}
