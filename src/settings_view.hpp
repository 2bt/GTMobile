#pragma once


#define SETTINGS(X) \
    X(fullscreen_enabled,   bool, false) \
    X(keep_screen_on,       bool, false) \
    X(row_highlight,        int,  8) \
    X(row_height,           int,  15) \
    X(sampling_method,      int,  3) \
    X(register_write_order, int,  0)


namespace settings_view {

    struct Settings {
        #define X(n, t, d) t n = d;
        SETTINGS(X)
        #undef X
    };

    char const* get_setting_name(int i);
    int         get_setting_value(int i);
    void        set_setting_value(int i, int v);

    void            draw();
    Settings const& settings();
    Settings&       mutable_settings();
}
