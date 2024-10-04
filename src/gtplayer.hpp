#pragma once
#include <cstdint>
#include <array>
#include "gtsong.hpp"


namespace gt {


class Player {
public:
    gt::Song const& song;
    Player(gt::Song const& song);

    void play_song() { m_is_playing_req = true; }
    void stop_song() { m_is_playing_req = false; }
    bool is_playing() const { return m_is_playing_req; }

    bool get_loop() const { return m_loop_pattern_req; }
    void set_loop(bool loop) { m_loop_pattern_req = loop; }

    void get_chan_info(int c, int& song_pos, int& patt_pos, uint8_t& pattnum) const {
        song_pos = m_current_song_pos[c];
        patt_pos = m_current_patt_pos[c];
        pattnum  = m_channels[c].pattnum;
    }

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
        uint32_t pattptr;
        uint8_t  pattnum;
        uint8_t  songptr;
        uint8_t  repeat;
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

    // play options
    const int      m_multiplier       = 1;      // for multi speed
    const uint16_t m_adparam          = 0x0f00; // HR
    const bool     m_optimizepulse    = false;
    const bool     m_optimizerealtime = false;


    bool   m_is_playing       = false;
    bool   m_is_playing_req   = false;
    bool   m_loop_pattern     = false;
    bool   m_loop_pattern_req = false;

    int    m_start_song_pos = 0;
    int    m_start_patt_pos = 0;
    std::array<int, MAX_CHN> m_current_song_pos;
    std::array<int, MAX_CHN> m_current_patt_pos;


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
