#pragma once

namespace song_view {

    int  channel();
    int  song_position();
    int  cursor_instrument();
    void reset();
    void draw();
    void draw_pattern();

    bool get_follow();
    void toggle_follow();

} // namespace song_view
