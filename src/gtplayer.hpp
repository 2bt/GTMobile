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

    enum class Action { None, Start, RestartPattern, FastBackward, FastForward, Pause, Stop };

    void set_action(Action action) { m_action = action; }
    bool is_playing() const { return m_is_playing; }

    bool get_pattern_looping() const { return m_loop_pattern; }
    void set_pattern_loopping(bool loop) { m_loop_pattern = loop; }

    void play_test_note(int note, int ins, int chnnum);
    void release_note(int chnnum);

    void set_channel_active(int chnnum, bool active) { m_channels[chnnum].mute = !active; }
    bool is_channel_active(int chnnum) const { return !m_channels[chnnum].mute; }

    void play_routine();

    using Registers = std::array<uint8_t, 25>;
    Registers const& registers() const { return m_regs; }
    gt::Song const&  song() const { return *m_song; }

    // used to calculate song length in ticks
    int channel_loop_counter(int c) const { return m_channels[c].loop_counter; }
    int channel_tempo(int c) const {
        Channel const& chan = m_channels[c];
        if (chan.tempo >= 2) return chan.tempo;
        else                 return m_funktable[chan.tempo];
    }

private:
    void sequencer(int c, bool reset_current_patt_pos = true);

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
        int      loop_counter;
    };


    // play options
    static constexpr bool m_optimizepulse    = false;
    static constexpr bool m_optimizerealtime = false;

    gt::Song const* m_song;
    Registers       m_regs;
    Action          m_action;
    bool            m_is_playing;
    bool            m_loop_pattern;

public:
    std::array<int, MAX_CHN> m_start_song_pos;
    std::array<int, MAX_CHN> m_start_patt_pos;
    std::array<int, MAX_CHN> m_current_song_pos;
    std::array<int, MAX_CHN> m_current_patt_pos;
private:

    std::array<Channel, MAX_CHN> m_channels;
    uint8_t                      m_filterctrl;
    uint8_t                      m_filtertype;
    uint8_t                      m_filtercutoff;
    uint8_t                      m_filtertime;
    uint8_t                      m_filterptr;
    std::array<uint8_t, 2>       m_funktable;
    uint8_t                      m_masterfader;
};


} // namespace gt
