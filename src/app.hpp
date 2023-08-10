#pragma once
#include <cstdint>
#include <string>
#include "gtplayer.hpp"
#include "gtsong.hpp"



#define SETTINGS(X) \
    X(bool, play_in_background) \
    X(int,  row_highlight) \
    X(int,  row_height)


namespace app {

    struct Settings {
        #define X(t, n) t n;
        SETTINGS(X)
        #undef X
    };

    enum {
        MIXRATE           = 44100,
        CANVAS_WIDTH      = 360,
        CANVAS_MIN_HEIGHT = 590,
        BUTTON_WIDTH      = 20
    };

    enum class View {
        Project,
        Song,
        Instrument,
        Settings,
    };


    gt::Song&   song();
    gt::Player& player();
    int         canvas_height();
    void        set_view(View view);


    Settings const& settings();
    Settings& mutable_settings();
    void init();
    void free();
    void set_refresh_rate(float refresh_rate);
    void resize(int w, int h);
    void touch(int x, int y, bool pressed);
    void key(int key, int unicode);
    void draw();
    void audio_callback(int16_t* buffer, int length);
}
