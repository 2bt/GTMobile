#pragma once
#include "app.hpp"

namespace piano {

    enum {
        KEY_HALF_WIDTH   = 12,
        KEY_HALF_HEIGHT  = 30,
        PIANO_PAGE       = app::CANVAS_WIDTH / KEY_HALF_WIDTH,
        PIANO_STEP_COUNT = 14 * 8 - 4,
        HEIGHT           = app::BUTTON_HEIGHT + KEY_HALF_HEIGHT * 2 + + app::TAB_HEIGHT + gui::FRAME_WIDTH * 2,
    };


    void draw();
    int  instrument();
    void set_instrument(int i);
    int  note();
    bool note_on();

} // namespace piano
