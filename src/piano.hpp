#pragma once
#include "app.hpp"

namespace piano {

    enum {
        KEY_HALF_WIDTH   = 12,
        KEY_HALF_HEIGHT  = 30,
        PIANO_PAGE       = app::CANVAS_WIDTH / KEY_HALF_WIDTH,
        PIANO_STEP_COUNT = 14 * 8 - 4,
        HEIGHT           = app::BUTTON_WIDTH + KEY_HALF_HEIGHT * 2 + + app::TAB_WIDTH,
    };


    bool draw(bool* follow=nullptr);
    int  instrument();
    int  note();

} // namespace piano
