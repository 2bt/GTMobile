#pragma once
#include <cstdint>


namespace app {

    enum {
        MIXRATE    = 44100,
        WIDTH      = 360,
        MIN_HEIGHT = 590,
    };

    void init();
    void free();
    void resize(int w, int h);
    void touch(int x, int y, bool pressed);
    void key(int key, int unicode);
    void draw();
    void audio_callback(int16_t* buffer, int length);
}
