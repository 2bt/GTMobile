#pragma once
#include <cstdint>
#include <string>
#include "gtplayer.hpp"
#include "gtsong.hpp"


namespace app {

    enum {
        MIXRATE           = 44100,
        CANVAS_WIDTH      = 360,
        CANVAS_MIN_HEIGHT = 590,
        BUTTON_HEIGHT     = 20,
        TAB_HEIGHT        = 30,
    };

    enum class View {
        project,
        song,
        instrument,
        settings,
    };


    gt::Song&   song();
    gt::Player& player();
    int         canvas_height();
    void        set_view(View view);


    void init();
    void free();
    void resize(int w, int h);
    void touch(int x, int y, bool pressed);
    void key(int key, int unicode);
    void draw();
    void audio_callback(int16_t* buffer, int length);
}
