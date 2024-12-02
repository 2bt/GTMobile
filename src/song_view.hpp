#pragma once

namespace song_view {

    int  channel();
    int  song_position();
    void reset();
    void draw();

    bool get_follow();
    void toggle_follow();

} // namespace song_view
