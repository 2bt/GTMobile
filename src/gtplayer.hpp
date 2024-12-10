#pragma once
#include <cstdint>
#include <array>
#include "gtsong.hpp"


namespace gt {


uint16_t get_freq(int note);


class Player {
public:
    Player(gt::Song const& song);

    void reset();

    void play_song() { m_action = Action::start; }
    void stop_song() { m_action = Action::stop; }
    void pause_song() { m_action = Action::pause; }
    bool is_playing() const { return m_is_playing; }

    bool get_loop() const { return m_loop_pattern_req; }
    void set_loop(bool loop) { m_loop_pattern_req = loop; }

    void play_test_note(int note, int ins, int chnnum);
    void release_note(int chnnum);

    void set_channel_active(int chnnum, bool active) { m_channels[chnnum].mute = !active; }
    bool is_channel_active(int chnnum) const { return !m_channels[chnnum].mute; }

    void play_routine();
    std::array<uint8_t, 25> regs;

private:
    void sequencer(int c);

    struct Channel {
        uint8_t  trans;
        uint8_t  instr;
        uint8_t  note;
        uint8_t  lastnote;
        uint8_t  newnote;
        int      pattptr;
        uint8_t  pattnum;
        uint8_t  songptr;
        uint16_t freq;
        uint8_t  gate;
        uint8_t  wave;
        uint16_t pulse;
        uint8_t  ptr[2];
        uint8_t  pulsetime;
        uint8_t  wavetime;
        uint8_t  vibtime;
        uint8_t  vibdelay;
        uint8_t  command;
        uint8_t  cmddata;
        uint8_t  newcommand;
        uint8_t  newcmddata;
        uint8_t  tick;
        uint8_t  tempo;
        uint8_t  mute;
        uint8_t  gatetimer;
    };

    enum class Action { none, start, pause, stop };

    // play options
    static constexpr int      m_multiplier       = 1;      // for multi speed
    static constexpr uint16_t m_adparam          = 0x0f00; // HR
    static constexpr bool     m_optimizepulse    = false;
    static constexpr bool     m_optimizerealtime = false;

    gt::Song const* m_song;
    Action          m_action           = Action::none;
    bool            m_is_playing       = false;
    bool            m_loop_pattern     = false;
    bool            m_loop_pattern_req = false;

public:
    std::array<int, MAX_CHN> m_start_song_pos   = {};
    std::array<int, MAX_CHN> m_start_patt_pos   = {};
    std::array<int, MAX_CHN> m_current_song_pos = {};
    std::array<int, MAX_CHN> m_current_patt_pos = {};
private:

    std::array<Channel, MAX_CHN> m_channels     = {};
    uint8_t                      m_filterctrl   = 0;
    uint8_t                      m_filtertype   = 0;
    uint8_t                      m_filtercutoff = 0;
    uint8_t                      m_filtertime   = 0;
    uint8_t                      m_filterptr    = 0;
    uint8_t                      m_funktable[2];
    uint8_t                      m_masterfader  = 0x0f;

};


} // namespace gt
