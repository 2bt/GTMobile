#pragma once
#include <cstdint>


namespace sid {
    void init(int mixrate);
    void write(int reg, uint8_t value);
    void mix(int16_t* buffer, int length);

    void update_state();
    float chan_level(int c);
}
