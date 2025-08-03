#pragma once
#include "app.hpp"

namespace piano {

    enum {
        KEY_HALF_WIDTH   = 12,
        KEY_HALF_HEIGHT  = 30,
        PIANO_PAGE       = app::CANVAS_WIDTH / KEY_HALF_WIDTH,
        PIANO_STEP_COUNT = 14 * 8 - 4,
        HEIGHT           = app::BUTTON_HEIGHT + KEY_HALF_HEIGHT * 2 + app::TAB_HEIGHT + gui::FRAME_WIDTH * 2,
    };


    struct InstrButtonProps {
        bool             disabled;
        gui::ButtonStyle style;
        bool             is_active;
    };

    void reset();
    void draw_instrument_window(char const* title,
                                std::function<InstrButtonProps(int)> const& get_props,
                                std::function<void(int)> const& pressed_cb,
                                std::function<void(int)> const& draw_bottom);

    void draw();
    int  instrument();
    void set_instrument(int i);
    int  note();
    bool note_on();

} // namespace piano
