#pragma once
#include <cstdint>
#include "gtplayer.hpp"
#include "gtsong.hpp"


namespace app {

    enum {
        MIXRATE           = 44100,

        CANVAS_WIDTH      = 360,
        CANVAS_MIN_HEIGHT = 590,

        KEYCODE_ENTER     = 66,
        KEYCODE_DEL       = 67,
    };

    gt::Song&   song();
    gt::Player& player();
    int  canvas_height();

    void init();
    void free();
    void set_refresh_rate(float refresh_rate);
    void resize(int w, int h);
    void touch(int x, int y, bool pressed);
    void key(int key, int unicode);
    void draw();
    void audio_callback(int16_t* buffer, int length);
}
