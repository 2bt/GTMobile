#pragma once
#include <cstdint>
#include <array>
#include "gtsong.hpp"


namespace gt {


class Player {
public:
    gt::Song const& song;
    Player(gt::Song const& song);

    enum Mode {
        PLAY_FOO       = 0,
        PLAY_BEGINNING = 1,
        PLAY_POS       = 2,
        PLAY_PATTERN   = 3,
        PLAY_STOP      = 4,
        PLAY_STOPPED   = 128,
    };

    void init_song(int num, Mode mode, int pattpos = 0);
    void stop_song();
    void play_routine();


    void get_chan_info(int chnnum, uint8_t& songptr, uint8_t& pattnum, uint32_t& pattptr) const {
        Channel const& c = m_channels[chnnum];
        songptr = c.songptr;
        pattnum = c.pattnum;
        pattptr = c.pattptr;
    }


    // TODO: test
    void release_note(int chnnum);
    void play_test_note(int note, int ins, int chnnum);

    void mute_channel(int chnnum) { m_channels[chnnum].mute ^= 1; }
    int  is_playing() const { return m_songinit != PLAY_STOPPED; }

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
        uint8_t  advance;
        uint8_t  gatetimer;
    };

    // play options
    int      m_multiplier       = 1;      // for multi speed
    uint16_t m_adparam          = 0x0f00; // HR
    bool     m_optimizepulse    = false;
    bool     m_optimizerealtime = false;

    int      m_espos[MAX_CHN]; // PLAY_POS     -> start
    int      m_esend[MAX_CHN]; // PLAY_POS     -> end
    int      m_epnum[MAX_CHN]; // PLAY_PATTERN -> pattern number

    // state
    Mode                         m_songinit     = PLAY_FOO;
    Mode                         m_lastsonginit = PLAY_FOO;
    int                          m_startpattpos = 0;
    std::array<Channel, MAX_CHN> m_channels     = {};
    uint8_t                      m_filterctrl   = 0;
    uint8_t                      m_filtertype   = 0;
    uint8_t                      m_filtercutoff = 0;
    uint8_t                      m_filtertime   = 0;
    uint8_t                      m_filterptr    = 0;
    uint8_t                      m_funktable[2];
    uint8_t                      m_masterfader  = 0x0f;
    int                          m_psnum        = 0; // song number

};


} // namespace gt
