#pragma once
#include <cstdint>
#include <string>
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

    enum class View {
        project,
        song,
        instrument,
        settings,
    };

    template<class T>
    bool slider(int width, T& value, int min, int max, void const* addr=nullptr) {
        int v = value;
        int old_v = value;
        gui::drag_bar_style(gui::DragBarStyle::Normal);
        gui::item_size({ width - BUTTON_HEIGHT * 2, BUTTON_HEIGHT });
        gui::id(addr ? addr : &value);
        gui::horizontal_drag_bar(v, min, max, 1);
        gui::same_line();
        gui::button_style(gui::ButtonStyle::Normal);
        gui::item_size(BUTTON_HEIGHT);
        if (gui::button(gui::Icon::Left)) v = std::max(min, v - 1);
        gui::same_line();
        if (gui::button(gui::Icon::Right)) v = std::min(max, v + 1);
        value = v;
        return v != old_v;
    }




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
