#pragma once
#include <cstdint>
#include <string>
#include <cstring>
#include <functional>
#include "gtplayer.hpp"
#include "gtsong.hpp"
#include "gui.hpp"
#include "sid.hpp"


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


    class Mixer {
    public:
        Mixer(gt::Player& player, Sid& sid) : m_player(player), m_sid(sid) {}
        void mix(int16_t* buffer, int length);
    private:
        gt::Player& m_player;
        Sid&        m_sid;
        int         m_cycles_to_next_write = 0;
        int         m_reg                  = 0;
    };


    gt::Song&          song();
    gt::Player&        player();
    Sid&               sid();
    int                canvas_height();
    void               set_storage_dir(std::string const& storage_dir);
    std::string const& storage_dir();

    using ConfirmCallback = std::function<void(bool)>;
    void draw_confirm();
    void confirm(std::string msg, ConfirmCallback cb);


    void init();
    void free();
    void resize(int w, int h);
    void touch(int x, int y, bool pressed);
    void key(int key, int unicode);
    void draw();
    void audio_callback(int16_t* buffer, int length);

}
