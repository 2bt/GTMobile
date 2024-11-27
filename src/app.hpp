#pragma once
#include <cstdint>
#include <string>
#include <cstring>
#include "gtplayer.hpp"
#include "gtsong.hpp"
#include "gui.hpp"


namespace app {

    enum {
        MIXRATE           = 44100,
        CANVAS_WIDTH      = 360,
        CANVAS_MIN_HEIGHT = 590,
        BUTTON_HEIGHT     = 30,
        TAB_HEIGHT        = 40,
        SCROLL_WIDTH      = 20,
        MAX_ROW_HEIGHT    = 20,
    };


    gt::Song&   song();
    gt::Player& player();
    int         canvas_height();


    void init();
    void free();
    void resize(int w, int h);
    void touch(int x, int y, bool pressed);
    void key(int key, int unicode);
    void draw();
    void audio_callback(int16_t* buffer, int length);
}
