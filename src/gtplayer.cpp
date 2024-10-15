#include <cstdio>
#include <cstring>
#include <cassert>
#include "gtplayer.hpp"


namespace gt {
namespace {

constexpr uint8_t FREQ_LO[] = {
    0x17, 0x27, 0x39, 0x4b, 0x5f, 0x74, 0x8a, 0xa1, 0xba, 0xd4, 0xf0, 0x0e, 0x2d, 0x4e, 0x71, 0x96,
    0xbe, 0xe8, 0x14, 0x43, 0x74, 0xa9, 0xe1, 0x1c, 0x5a, 0x9c, 0xe2, 0x2d, 0x7c, 0xcf, 0x28, 0x85,
    0xe8, 0x52, 0xc1, 0x37, 0xb4, 0x39, 0xc5, 0x5a, 0xf7, 0x9e, 0x4f, 0x0a, 0xd1, 0xa3, 0x82, 0x6e,
    0x68, 0x71, 0x8a, 0xb3, 0xee, 0x3c, 0x9e, 0x15, 0xa2, 0x46, 0x04, 0xdc, 0xd0, 0xe2, 0x14, 0x67,
    0xdd, 0x79, 0x3c, 0x29, 0x44, 0x8d, 0x08, 0xb8, 0xa1, 0xc5, 0x28, 0xcd, 0xba, 0xf1, 0x78, 0x53,
    0x87, 0x1a, 0x10, 0x71, 0x42, 0x89, 0x4f, 0x9b, 0x74, 0xe2, 0xf0, 0xa6, 0x0e, 0x33, 0x20, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
constexpr uint8_t FREQ_HI[] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x06, 0x06,
    0x06, 0x07, 0x07, 0x08, 0x08, 0x09, 0x09, 0x0a, 0x0a, 0x0b, 0x0c, 0x0d, 0x0d, 0x0e, 0x0f, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x17, 0x18, 0x1a, 0x1b, 0x1d, 0x1f, 0x20, 0x22, 0x24, 0x27, 0x29,
    0x2b, 0x2e, 0x31, 0x34, 0x37, 0x3a, 0x3e, 0x41, 0x45, 0x49, 0x4e, 0x52, 0x57, 0x5c, 0x62, 0x68,
    0x6e, 0x75, 0x7c, 0x83, 0x8b, 0x93, 0x9c, 0xa5, 0xaf, 0xb9, 0xc4, 0xd0, 0xdd, 0xea, 0xf8, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

} // namespace


Player::Player(Song const& song) : m_song(song) {
    reset();
}

void Player::reset() {
    for (int c = 0; c < MAX_CHN; c++) {
        m_channels[c].trans = 0;
        m_channels[c].instr = 1;
        m_channels[c].tempo = 6 * m_multiplier - 1;
    }
    m_funktable[0] = 9 * m_multiplier - 1;
    m_funktable[1] = 6 * m_multiplier - 1;
}

void Player::release_note(int chnnum) {
    m_channels[chnnum].gate    = 0xfe;
    m_channels[chnnum].newnote = 0;
}

void Player::play_test_note(int note, int ins, int chnnum) {
    if (note == KEYON) return;
    if (note == REST || note == KEYOFF) {
        release_note(chnnum);
        return;
    }

    if (!(m_song.instruments[ins].gatetimer & 0x40)) {
        m_channels[chnnum].gate = 0xfe; // keyoff
        if (!(m_song.instruments[ins].gatetimer & 0x80)) {
            regs[0x5 + chnnum * 7] = m_adparam >> 8; // hardrestart
            regs[0x6 + chnnum * 7] = m_adparam & 0xff;
        }
    }

    m_channels[chnnum].instr   = ins;
    m_channels[chnnum].newnote = note;
    if (!m_is_playing) {
        m_channels[chnnum].tick      = (m_song.instruments[ins].gatetimer & 0x3f) + 1;
        m_channels[chnnum].gatetimer = m_song.instruments[ins].gatetimer & 0x3f;
    }
}


void Player::sequencer(int c) {
    Channel&    chan  = m_channels[c];
    auto const& order = m_song.song_order[c];

    // song loop
    if (chan.songptr >= m_song.song_len) {
        chan.songptr = m_song.song_loop;
    }
    assert(chan.songptr < m_song.song_len);

    // store current song position
    m_current_song_pos[c] = chan.songptr;

    chan.trans   = order[chan.songptr].trans;
    chan.pattnum = order[chan.songptr].pattnum;
    chan.songptr++;

    // check for illegal pattern
    if (chan.pattnum >= MAX_PATT) {
        stop_song();
        chan.pattnum = 0;
    }
}


void Player::play_routine() {
    m_loop_pattern = m_loop_pattern_req;

    if (m_action == Action::pause || m_action == Action::stop) {
        m_is_playing = false;
        for (int c = 0; c < MAX_CHN; c++) {
            Channel& chan = m_channels[c];
            chan.command    = 0;
            chan.cmddata    = 0;
            chan.newcommand = 0;
            chan.newcmddata = 0;
            chan.ptr[WTBL]  = 0;
            chan.newnote    = 0;
            chan.tick       = 6 * m_multiplier - 1;
            chan.gatetimer  = m_song.instruments[1].gatetimer & 0x3f;
            chan.gate       = 0xfe;    // note off
            regs[0x6 + 7 * c] &= 0xf0; // fast release
            if (m_action == Action::stop && chan.tempo < 2) chan.tick = 0;
        }
        if (m_action == Action::pause) {
            m_start_song_pos = m_current_song_pos;
            m_start_patt_pos = m_current_patt_pos;
        }
        else {
            m_current_song_pos = m_start_song_pos;
            m_current_patt_pos = m_start_patt_pos;
        }
        m_action = Action::none;
    }

    if (m_action == Action::start) {
        m_action = Action::none;
        m_is_playing = true;

        m_filterctrl = 0;
        m_filterptr  = 0;
        for (int c = 0; c < MAX_CHN; c++) {
            Channel& chan = m_channels[c];
            chan.command    = 0;
            chan.cmddata    = 0;
            chan.newcommand = 0;
            chan.newcmddata = 0;
            chan.wave       = 0;
            chan.ptr[WTBL]  = 0;
            chan.newnote    = 0;
            chan.tick       = 6 * m_multiplier - 1;
            chan.gatetimer  = m_song.instruments[1].gatetimer & 0x3f;
            chan.songptr    = m_start_song_pos[c];
            chan.pattptr    = m_start_patt_pos[c];
            sequencer(c);
        }
    }

    if (m_filterptr) {
        // filter jump
        if (m_song.ltable[FTBL][m_filterptr - 1] == 0xff) {
            m_filterptr = m_song.rtable[FTBL][m_filterptr - 1];
            if (!m_filterptr) goto FILTERSTOP;
        }

        if (!m_filtertime) {
            // filter set
            if (m_song.ltable[FTBL][m_filterptr - 1] >= 0x80) {
                m_filtertype = m_song.ltable[FTBL][m_filterptr - 1] & 0x70;
                m_filterctrl = m_song.rtable[FTBL][m_filterptr - 1];
                m_filterptr++;
                // can be combined with cutoff set
                if (m_song.ltable[FTBL][m_filterptr - 1] == 0x00) {
                    m_filtercutoff = m_song.rtable[FTBL][m_filterptr - 1];
                    m_filterptr++;
                }
            }
            else {
                // new modulation step
                if (m_song.ltable[FTBL][m_filterptr - 1]) m_filtertime = m_song.ltable[FTBL][m_filterptr - 1];
                else {
                    // cutoff set
                    m_filtercutoff = m_song.rtable[FTBL][m_filterptr - 1];
                    m_filterptr++;
                }
            }
        }
        // filter modulation
        if (m_filtertime) {
            m_filtercutoff += m_song.rtable[FTBL][m_filterptr - 1];
            m_filtertime--;
            if (!m_filtertime) m_filterptr++;
        }
    }

FILTERSTOP:
    regs[0x15] = 0x00;
    regs[0x16] = m_filtercutoff;
    regs[0x17] = m_filterctrl;
    regs[0x18] = m_filtertype | m_masterfader;

    for (int c = 0; c < MAX_CHN; c++) {
        Channel&          chan  = m_channels[c];
        Instrument const& instr = m_song.instruments[chan.instr];

        // decrease tick
        chan.tick--;
        if (!chan.tick) goto TICK0;

        // tick N
        // reload counter
        if (chan.tick >= 0x80) {
            if (chan.tempo >= 2) chan.tick = chan.tempo;
            else {
                // set funktempo, switch between 2 values
                chan.tick = m_funktable[chan.tempo];
                chan.tempo ^= 1;
            }
            // check for illegally high gatetimer and stop the song in this case
            if (chan.gatetimer > chan.tick) stop_song();
        }
        goto WAVEEXEC;

TICK0:
        // tick 0
        if (m_is_playing && chan.pattptr == 0x7fffffff) {
            chan.pattptr = 0;
            if (!m_loop_pattern) sequencer(c);
        }

        // get gatetimer compare-value
        chan.gatetimer = instr.gatetimer & 0x3f;

        // new note init
        if (chan.newnote) {
            chan.note     = chan.newnote - FIRSTNOTE;
            chan.command  = 0;
            chan.vibdelay = instr.vibdelay;
            chan.cmddata  = instr.ptr[STBL];
            if (chan.newcommand != CMD_TONEPORTA) {
                if (instr.firstwave) {
                    if (instr.firstwave >= 0xfe) chan.gate = instr.firstwave;
                    else {
                        chan.wave = instr.firstwave;
                        chan.gate = 0xff;
                    }
                }

                chan.ptr[WTBL] = instr.ptr[WTBL];

                if (chan.ptr[WTBL]) {
                    // stop the song in case of jumping into a jump
                    if (m_song.ltable[WTBL][chan.ptr[WTBL] - 1] == 0xff) stop_song();
                }
                if (instr.ptr[PTBL]) {
                    chan.ptr[PTBL] = instr.ptr[PTBL];
                    chan.pulsetime = 0;
                    if (chan.ptr[PTBL]) {
                        // stop the song in case of jumping into a jump
                        if (m_song.ltable[PTBL][chan.ptr[PTBL] - 1] == 0xff) stop_song();
                    }
                }
                if (instr.ptr[FTBL]) {
                    m_filterptr  = instr.ptr[FTBL];
                    m_filtertime = 0;
                    if (m_filterptr) {
                        // stop the song in case of jumping into a jump
                        if (m_song.ltable[FTBL][m_filterptr - 1] == 0xff) stop_song();
                    }
                }
                regs[0x5 + 7 * c] = instr.ad;
                regs[0x6 + 7 * c] = instr.sr;
            }
        }

        // tick 0 effects

        switch (chan.newcommand) {
        case CMD_DONOTHING:
            chan.command = 0;
            chan.cmddata = instr.ptr[STBL];
            break;

        case CMD_PORTAUP:
        case CMD_PORTADOWN:
            chan.vibtime = 0;
            chan.command = chan.newcommand;
            chan.cmddata = chan.newcmddata;
            break;

        case CMD_TONEPORTA:
        case CMD_VIBRATO:
            chan.command = chan.newcommand;
            chan.cmddata = chan.newcmddata;
            break;

        case CMD_SETAD: regs[0x5 + 7 * c] = chan.newcmddata; break;

        case CMD_SETSR: regs[0x6 + 7 * c] = chan.newcmddata; break;

        case CMD_SETWAVE: chan.wave = chan.newcmddata; break;

        case CMD_SETWAVEPTR:
            chan.ptr[WTBL] = chan.newcmddata;
            chan.wavetime  = 0;
            if (chan.ptr[WTBL]) {
                // stop the song in case of jumping into a jump
                if (m_song.ltable[WTBL][chan.ptr[WTBL] - 1] == 0xff) stop_song();
            }
            break;

        case CMD_SETPULSEPTR:
            chan.ptr[PTBL] = chan.newcmddata;
            chan.pulsetime = 0;
            if (chan.ptr[PTBL]) {
                // stop the song in case of jumping into a jump
                if (m_song.ltable[PTBL][chan.ptr[PTBL] - 1] == 0xff) stop_song();
            }
            break;

        case CMD_SETFILTERPTR:
            m_filterptr  = chan.newcmddata;
            m_filtertime = 0;
            if (m_filterptr) {
                // stop the song in case of jumping into a jump
                if (m_song.ltable[FTBL][m_filterptr - 1] == 0xff) stop_song();
            }
            break;

        case CMD_SETFILTERCTRL:
            m_filterctrl = chan.newcmddata;
            if (!m_filterctrl) m_filterptr = 0;
            break;

        case CMD_SETFILTERCUTOFF: m_filtercutoff = chan.newcmddata; break;

        case CMD_SETMASTERVOL:
            if (chan.newcmddata < 0x10) m_masterfader = chan.newcmddata;
            break;

        case CMD_FUNKTEMPO:
            if (chan.newcmddata) {
                m_funktable[0] = m_song.ltable[STBL][chan.newcmddata - 1] - 1;
                m_funktable[1] = m_song.rtable[STBL][chan.newcmddata - 1] - 1;
            }
            m_channels[0].tempo = 0;
            m_channels[1].tempo = 0;
            m_channels[2].tempo = 0;
            break;

        case CMD_SETTEMPO: {
            uint8_t newtempo = chan.newcmddata & 0x7f;

            if (newtempo >= 3) newtempo--;
            if (chan.newcmddata >= 0x80) chan.tempo = newtempo;
            else {
                m_channels[0].tempo = newtempo;
                m_channels[1].tempo = newtempo;
                m_channels[2].tempo = newtempo;
            }
        } break;
        }
        if (chan.newnote) {
            chan.newnote = 0;
            if (chan.newcommand != CMD_TONEPORTA) goto NEXTCHN;
        }

WAVEEXEC:
        if (chan.ptr[WTBL]) {
            uint8_t wave = m_song.ltable[WTBL][chan.ptr[WTBL] - 1];
            uint8_t note = m_song.rtable[WTBL][chan.ptr[WTBL] - 1];

            if (wave > WAVELASTDELAY) {
                // normal waveform values
                if (wave < WAVESILENT) chan.wave = wave;
                // values without waveform selected
                if ((wave >= WAVESILENT) && (wave <= WAVELASTSILENT)) chan.wave = wave & 0xf;
                // command execution from wavetable
                if ((wave >= WAVECMD) && (wave <= WAVELASTCMD)) {
                    uint8_t param = m_song.rtable[WTBL][chan.ptr[WTBL] - 1];
                    switch (wave & 0xf) {
                    case CMD_DONOTHING:
                    case CMD_SETWAVEPTR:
                    case CMD_FUNKTEMPO: stop_song(); break;

                    case CMD_PORTAUP: {
                        uint16_t speed = 0;
                        if (param) {
                            speed = (m_song.ltable[STBL][param - 1] << 8) | m_song.rtable[STBL][param - 1];
                        }
                        if (speed >= 0x8000) {
                            speed = FREQ_LO[chan.lastnote + 1] | (FREQ_HI[chan.lastnote + 1] << 8);
                            speed -= FREQ_LO[chan.lastnote] | (FREQ_HI[chan.lastnote] << 8);
                            speed >>= m_song.rtable[STBL][param - 1];
                        }
                        chan.freq += speed;
                    } break;

                    case CMD_PORTADOWN: {
                        uint16_t speed = 0;
                        if (param) {
                            speed = (m_song.ltable[STBL][param - 1] << 8) | m_song.rtable[STBL][param - 1];
                        }
                        if (speed >= 0x8000) {
                            speed = FREQ_LO[chan.lastnote + 1] | (FREQ_HI[chan.lastnote + 1] << 8);
                            speed -= FREQ_LO[chan.lastnote] | (FREQ_HI[chan.lastnote] << 8);
                            speed >>= m_song.rtable[STBL][param - 1];
                        }
                        chan.freq -= speed;
                    } break;

                    case CMD_TONEPORTA: {
                        uint16_t targetfreq = FREQ_LO[chan.note] | (FREQ_HI[chan.note] << 8);
                        uint16_t speed      = 0;

                        if (!param) {
                            chan.freq    = targetfreq;
                            chan.vibtime = 0;
                        }
                        else {
                            speed = (m_song.ltable[STBL][param - 1] << 8) | m_song.rtable[STBL][param - 1];
                            if (speed >= 0x8000) {
                                speed = FREQ_LO[chan.lastnote + 1] | (FREQ_HI[chan.lastnote + 1] << 8);
                                speed -= FREQ_LO[chan.lastnote] | (FREQ_HI[chan.lastnote] << 8);
                                speed >>= m_song.rtable[STBL][param - 1];
                            }
                            if (chan.freq < targetfreq) {
                                chan.freq += speed;
                                if (chan.freq > targetfreq) {
                                    chan.freq    = targetfreq;
                                    chan.vibtime = 0;
                                }
                            }
                            if (chan.freq > targetfreq) {
                                chan.freq -= speed;
                                if (chan.freq < targetfreq) {
                                    chan.freq    = targetfreq;
                                    chan.vibtime = 0;
                                }
                            }
                        }
                    } break;

                    case CMD_VIBRATO: {
                        uint16_t speed    = 0;
                        uint8_t  cmpvalue = 0;

                        if (param) {
                            cmpvalue = m_song.ltable[STBL][param - 1];
                            speed    = m_song.rtable[STBL][param - 1];
                        }
                        if (cmpvalue >= 0x80) {
                            cmpvalue &= 0x7f;
                            speed = FREQ_LO[chan.lastnote + 1] | (FREQ_HI[chan.lastnote + 1] << 8);
                            speed -= FREQ_LO[chan.lastnote] | (FREQ_HI[chan.lastnote] << 8);
                            speed >>= m_song.rtable[STBL][param - 1];
                        }

                        if ((chan.vibtime < 0x80) && (chan.vibtime > cmpvalue)) chan.vibtime ^= 0xff;
                        chan.vibtime += 0x02;
                        if (chan.vibtime & 0x01) chan.freq -= speed;
                        else                     chan.freq += speed;
                    } break;

                    case CMD_SETAD: regs[0x5 + 7 * c] = param; break;

                    case CMD_SETSR:
                        regs[0x6 + 7 * c] = param;
                        break;

                    case CMD_SETWAVE: chan.wave = param; break;

                    case CMD_SETPULSEPTR:
                        chan.ptr[PTBL] = param;
                        chan.pulsetime = 0;
                        if (chan.ptr[PTBL]) {
                            // stop the song in case of jumping into a jump
                            if (m_song.ltable[PTBL][chan.ptr[PTBL] - 1] == 0xff) stop_song();
                        }
                        break;

                    case CMD_SETFILTERPTR:
                        m_filterptr  = param;
                        m_filtertime = 0;
                        if (m_filterptr) {
                            // stop the song in case of jumping into a jump
                            if (m_song.ltable[FTBL][m_filterptr - 1] == 0xff) stop_song();
                        }
                        break;

                    case CMD_SETFILTERCTRL:
                        m_filterctrl = param;
                        if (!m_filterctrl) m_filterptr = 0;
                        break;

                    case CMD_SETFILTERCUTOFF: m_filtercutoff = param; break;

                    case CMD_SETMASTERVOL:
                        if (param < 0x10) m_masterfader = param;
                        break;
                    }
                }
            }
            else {
                // wavetable delay
                if (chan.wavetime != wave) {
                    chan.wavetime++;
                    goto TICKNEFFECTS;
                }
            }

            chan.wavetime = 0;
            chan.ptr[WTBL]++;
            // wavetable jump
            if (m_song.ltable[WTBL][chan.ptr[WTBL] - 1] == 0xff) {
                chan.ptr[WTBL] = m_song.rtable[WTBL][chan.ptr[WTBL] - 1];
            }

            if (wave >= WAVECMD && wave <= WAVELASTCMD) goto PULSEEXEC;

            if (note != 0x80) {
                if (note < 0x80) note += chan.note;
                note &= 0x7f;
                chan.freq     = FREQ_LO[note] | (FREQ_HI[note] << 8);
                chan.vibtime  = 0;
                chan.lastnote = note;
                goto PULSEEXEC;
            }
        }

TICKNEFFECTS:
        // tick N command
        if (!m_optimizerealtime || chan.tick > 0) {
            switch (chan.command) {
            case CMD_PORTAUP: {
                uint16_t speed = 0;
                if (chan.cmddata) {
                    speed = (m_song.ltable[STBL][chan.cmddata - 1] << 8) | m_song.rtable[STBL][chan.cmddata - 1];
                }
                if (speed >= 0x8000) {
                    speed = FREQ_LO[chan.lastnote + 1] | (FREQ_HI[chan.lastnote + 1] << 8);
                    speed -= FREQ_LO[chan.lastnote] | (FREQ_HI[chan.lastnote] << 8);
                    speed >>= m_song.rtable[STBL][chan.cmddata - 1];
                }
                chan.freq += speed;
            } break;

            case CMD_PORTADOWN: {
                uint16_t speed = 0;
                if (chan.cmddata) {
                    speed = (m_song.ltable[STBL][chan.cmddata - 1] << 8) | m_song.rtable[STBL][chan.cmddata - 1];
                }
                if (speed >= 0x8000) {
                    speed = FREQ_LO[chan.lastnote + 1] | (FREQ_HI[chan.lastnote + 1] << 8);
                    speed -= FREQ_LO[chan.lastnote] | (FREQ_HI[chan.lastnote] << 8);
                    speed >>= m_song.rtable[STBL][chan.cmddata - 1];
                }
                chan.freq -= speed;
            } break;

            case CMD_DONOTHING:
                if (!chan.cmddata || !chan.vibdelay) break;
                if (chan.vibdelay > 1) {
                    chan.vibdelay--;
                    break;
                }
            case CMD_VIBRATO: {
                uint16_t speed    = 0;
                uint8_t  cmpvalue = 0;

                if (chan.cmddata) {
                    cmpvalue = m_song.ltable[STBL][chan.cmddata - 1];
                    speed    = m_song.rtable[STBL][chan.cmddata - 1];
                }
                if (cmpvalue >= 0x80) {
                    cmpvalue &= 0x7f;
                    speed = FREQ_LO[chan.lastnote + 1] | (FREQ_HI[chan.lastnote + 1] << 8);
                    speed -= FREQ_LO[chan.lastnote] | (FREQ_HI[chan.lastnote] << 8);
                    speed >>= m_song.rtable[STBL][chan.cmddata - 1];
                }

                if (chan.vibtime < 0x80 && chan.vibtime > cmpvalue) chan.vibtime ^= 0xff;
                chan.vibtime += 0x02;
                if (chan.vibtime & 0x01) chan.freq -= speed;
                else                     chan.freq += speed;
            } break;

            case CMD_TONEPORTA: {
                uint16_t targetfreq = FREQ_LO[chan.note] | (FREQ_HI[chan.note] << 8);
                uint16_t speed      = 0;

                if (!chan.cmddata) {
                    chan.freq    = targetfreq;
                    chan.vibtime = 0;
                }
                else {
                    speed = (m_song.ltable[STBL][chan.cmddata - 1] << 8) | m_song.rtable[STBL][chan.cmddata - 1];
                    if (speed >= 0x8000) {
                        speed = FREQ_LO[chan.lastnote + 1] | (FREQ_HI[chan.lastnote + 1] << 8);
                        speed -= FREQ_LO[chan.lastnote] | (FREQ_HI[chan.lastnote] << 8);
                        speed >>= m_song.rtable[STBL][chan.cmddata - 1];
                    }
                    if (chan.freq < targetfreq) {
                        chan.freq += speed;
                        if (chan.freq > targetfreq) {
                            chan.freq    = targetfreq;
                            chan.vibtime = 0;
                        }
                    }
                    if (chan.freq > targetfreq) {
                        chan.freq -= speed;
                        if (chan.freq < targetfreq) {
                            chan.freq    = targetfreq;
                            chan.vibtime = 0;
                        }
                    }
                }
            } break;
            }
        }

PULSEEXEC:
        if (m_optimizepulse) {
            if (m_is_playing && chan.tick == chan.gatetimer) goto GETNEWNOTES;
        }

        if (chan.ptr[PTBL]) {
            // skip pulse when sequencer has been executed
            if (m_optimizepulse) {
                if (!chan.tick && !chan.pattptr) goto NEXTCHN;
            }

            // pulsetable jump
            if (m_song.ltable[PTBL][chan.ptr[PTBL] - 1] == 0xff) {
                chan.ptr[PTBL] = m_song.rtable[PTBL][chan.ptr[PTBL] - 1];
                if (!chan.ptr[PTBL]) goto PULSEEXEC;
            }

            if (!chan.pulsetime) {
                // set pulse
                if (m_song.ltable[PTBL][chan.ptr[PTBL] - 1] >= 0x80) {
                    chan.pulse = (m_song.ltable[PTBL][chan.ptr[PTBL] - 1] & 0xf) << 8;
                    chan.pulse |= m_song.rtable[PTBL][chan.ptr[PTBL] - 1];
                    chan.ptr[PTBL]++;
                }
                else {
                    chan.pulsetime = m_song.ltable[PTBL][chan.ptr[PTBL] - 1];
                }
            }
            // pulse modulation
            if (chan.pulsetime) {
                uint8_t speed = m_song.rtable[PTBL][chan.ptr[PTBL] - 1];
                if (speed < 0x80) {
                    chan.pulse += speed;
                    chan.pulse &= 0xfff;
                }
                else {
                    chan.pulse += speed;
                    chan.pulse -= 0x100;
                    chan.pulse &= 0xfff;
                }
                chan.pulsetime--;
                if (!chan.pulsetime) chan.ptr[PTBL]++;
            }
        }
        if (!m_is_playing || chan.tick != chan.gatetimer) goto NEXTCHN;

GETNEWNOTES:
        // new notes processing
        {
            m_current_patt_pos[c] = chan.pattptr; // store current pattern position

            auto row = m_song.patterns[chan.pattnum].rows[chan.pattptr];
            if (++chan.pattptr >= m_song.patterns[chan.pattnum].len) chan.pattptr = 0x7fffffff;
            if (row.instr) chan.instr = row.instr;
            chan.newcommand = row.command;
            chan.newcmddata = row.data;

            if (row.note == KEYOFF) chan.gate = 0xfe;
            if (row.note == KEYON) chan.gate = 0xff;
            if (row.note <= LASTNOTE) {
                chan.newnote = row.note + chan.trans;
                if (chan.newcommand != CMD_TONEPORTA) {
                    if (!(m_song.instruments[chan.instr].gatetimer & 0x40)) {
                        chan.gate = 0xfe;
                        if (!(m_song.instruments[chan.instr].gatetimer & 0x80)) {
                            regs[0x5 + 7 * c] = m_adparam >> 8;
                            regs[0x6 + 7 * c] = m_adparam & 0xff;
                        }
                    }
                }
            }
        }
NEXTCHN:
        regs[0x0 + 7 * c] = chan.freq & 0xff;
        regs[0x1 + 7 * c] = chan.freq >> 8;
        regs[0x2 + 7 * c] = chan.pulse & 0xfe;
        regs[0x3 + 7 * c] = chan.pulse >> 8;
        if (chan.mute) {
            regs[0x4 + 7 * c] = chan.wave & 0x08; // don't set test bit every time
        }
        else {
            regs[0x4 + 7 * c] = chan.wave & chan.gate;
        }
    }
}

} // namespace gt
