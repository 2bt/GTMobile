#pragma once


#define SETTINGS(X) \
    X(play_in_background, bool, false) \
    X(row_highlight,      int,  8) \
    X(row_height,         int,  15)


namespace settings_view {

    struct Settings {
        #define X(n, t, d) t n = d;
        SETTINGS(X)
        #undef X
    };

    void            draw();
    Settings const& settings();
    Settings&       mutable_settings();
}
