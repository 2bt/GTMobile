#include <cstdio>
#include <cstring>
#include <cassert>
#include "gtplayer.hpp"


namespace gt {
namespace {

uint8_t const FREQ_LO[] = {
    0x17,0x27,0x39,0x4b,0x5f,0x74,0x8a,0xa1,0xba,0xd4,0xf0,0x0e,
    0x2d,0x4e,0x71,0x96,0xbe,0xe8,0x14,0x43,0x74,0xa9,0xe1,0x1c,
    0x5a,0x9c,0xe2,0x2d,0x7c,0xcf,0x28,0x85,0xe8,0x52,0xc1,0x37,
    0xb4,0x39,0xc5,0x5a,0xf7,0x9e,0x4f,0x0a,0xd1,0xa3,0x82,0x6e,
    0x68,0x71,0x8a,0xb3,0xee,0x3c,0x9e,0x15,0xa2,0x46,0x04,0xdc,
    0xd0,0xe2,0x14,0x67,0xdd,0x79,0x3c,0x29,0x44,0x8d,0x08,0xb8,
    0xa1,0xc5,0x28,0xcd,0xba,0xf1,0x78,0x53,0x87,0x1a,0x10,0x71,
    0x42,0x89,0x4f,0x9b,0x74,0xe2,0xf0,0xa6,0x0e,0x33,0x20,0xff,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
uint8_t const FREQ_HI[] = {
    0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x02,
    0x02,0x02,0x02,0x02,0x02,0x02,0x03,0x03,0x03,0x03,0x03,0x04,
    0x04,0x04,0x04,0x05,0x05,0x05,0x06,0x06,0x06,0x07,0x07,0x08,
    0x08,0x09,0x09,0x0a,0x0a,0x0b,0x0c,0x0d,0x0d,0x0e,0x0f,0x10,
    0x11,0x12,0x13,0x14,0x15,0x17,0x18,0x1a,0x1b,0x1d,0x1f,0x20,
    0x22,0x24,0x27,0x29,0x2b,0x2e,0x31,0x34,0x37,0x3a,0x3e,0x41,
    0x45,0x49,0x4e,0x52,0x57,0x5c,0x62,0x68,0x6e,0x75,0x7c,0x83,
    0x8b,0x93,0x9c,0xa5,0xaf,0xb9,0xc4,0xd0,0xdd,0xea,0xf8,0xff,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

} // namespace


Player::Player(Song const& song) : song(song) {
    for (int c = 0; c < MAX_CHN; c++) {
        m_channels[c].trans = 0;
        m_channels[c].instr = 1;
        if (m_multiplier) m_channels[c].tempo = 6 * m_multiplier - 1;
        else              m_channels[c].tempo = 6 - 1;
    }
    if (m_multiplier) {
        m_funktable[0] = 9 * m_multiplier - 1;
        m_funktable[1] = 6 * m_multiplier - 1;
    }
    else {
        m_funktable[0] = 9 - 1;
        m_funktable[1] = 6 - 1;
    }

    // XXX: copied from songchanged()
    for (int c = 0; c < MAX_CHN; c++) {
        m_espos[c] = 0;
        m_esend[c] = 0;
        m_epnum[c] = c;
    }

    init_song(0, PLAY_STOP);
}

void Player::init_song(int num, Mode mode, int pattpos) {
    m_psnum        = num;
    m_songinit     = mode;
    m_startpattpos = pattpos;
}

void Player::stop_song() {
    //if (m_songinit != PLAY_STOPPED) m_songinit = PLAY_STOP;
    m_songinit = PLAY_STOP;
}

//void rewindsong() {
//    if (lastsonginit == PLAY_BEGINNING) lastsonginit = PLAY_POS;
//    initsong(psnum, lastsonginit);
//}

void Player::release_note(int chnnum) {
    m_channels[chnnum].gate = 0xfe;
    m_channels[chnnum].newnote = 0; // 2bt: I added this
}

void Player::play_test_note(int note, int ins, int chnnum) {
    if (note == KEYON) return;
    if (note == REST || note == KEYOFF) {
        release_note(chnnum);
        return;
    }

    if (!(song.instr[ins].gatetimer & 0x40)) {
        m_channels[chnnum].gate = 0xfe; // keyoff
        if (!(song.instr[ins].gatetimer & 0x80)) {
            regs[0x5 + chnnum * 7] = m_adparam >> 8; // hardrestart
            regs[0x6 + chnnum * 7] = m_adparam & 0xff;
        }
    }

    m_channels[chnnum].instr   = ins;
    m_channels[chnnum].newnote = note;
    if (m_songinit == PLAY_STOPPED) {
        m_channels[chnnum].tick      = (song.instr[ins].gatetimer & 0x3f) + 1;
        m_channels[chnnum].gatetimer = song.instr[ins].gatetimer & 0x3f;
    }
}


void Player::sequencer(int c) {
    Channel& chan = m_channels[c];
    if (m_songinit == PLAY_STOPPED || chan.pattptr != 0x7fffffff) return;
    chan.pattptr = m_startpattpos * 4;
    if (!chan.advance) return;
    // song loop
    if (song.songorder[m_psnum][c][chan.songptr] == LOOPSONG) {
        chan.songptr = song.songorder[m_psnum][c][chan.songptr + 1];
        if (chan.songptr >= song.songlen[m_psnum][c]) {
            stop_song();
            chan.songptr = 0;
            return;
        }
    }
    // transpose
    if (song.songorder[m_psnum][c][chan.songptr] >= TRANSDOWN &&
        song.songorder[m_psnum][c][chan.songptr] < LOOPSONG)
    {
        chan.trans = song.songorder[m_psnum][c][chan.songptr] - TRANSUP;
        chan.songptr++;
    }
    // repeat
    if (song.songorder[m_psnum][c][chan.songptr] >= REPEAT &&
        song.songorder[m_psnum][c][chan.songptr] < TRANSDOWN)
    {
        chan.repeat = song.songorder[m_psnum][c][chan.songptr] - REPEAT;
        chan.songptr++;
    }
    // pattern number
    chan.pattnum = song.songorder[m_psnum][c][chan.songptr];
    if (chan.repeat) chan.repeat--;
    else             chan.songptr++;

    // check for illegal pattern now
    if (chan.pattnum >= MAX_PATT) {
        stop_song();
        chan.pattnum = 0;
    }
    if (chan.pattptr >= (song.pattlen[chan.pattnum] * 4)) chan.pattptr = 0;

    // check for playback endpos
    if (m_lastsonginit != PLAY_BEGINNING &&
        m_esend[c] > 0 &&
        m_esend[c] > m_espos[c] &&
        chan.songptr > m_esend[c] &&
        m_espos[c] < song.songlen[m_psnum][c])
    {
        chan.songptr = m_espos[c];
    }
}


void Player::play_routine() {

    //if (songinit == PLAY_STOP) followplay = 0;

    if (m_songinit > 0 && m_songinit < PLAY_STOPPED) {
        m_lastsonginit = m_songinit;

        m_filterctrl = 0;
        m_filterptr  = 0;

        if (m_songinit == PLAY_POS || m_songinit == PLAY_PATTERN) {
            if (m_espos[0] >= song.songlen[m_psnum][0] ||
                m_espos[1] >= song.songlen[m_psnum][1] ||
                m_espos[2] >= song.songlen[m_psnum][2])
            {
                m_songinit = PLAY_BEGINNING;
            }
        }

        for (int c = 0; c < MAX_CHN; c++) {
            Channel& chan = m_channels[c];
            chan.songptr    = 0;
            chan.command    = 0;
            chan.cmddata    = 0;
            chan.newcommand = 0;
            chan.newcmddata = 0;
            chan.advance    = 1;
            chan.wave       = 0;
            chan.ptr[WTBL]  = 0;
            chan.newnote    = 0;
            chan.repeat     = 0;
            if (m_multiplier) chan.tick = 6 * m_multiplier - 1;
            else            chan.tick = 6 - 1;
            chan.gatetimer = song.instr[1].gatetimer & 0x3f;
            chan.pattptr   = 0x7fffffff;
            if (chan.tempo < 2) chan.tempo = 0;

            switch (m_songinit) {
            case PLAY_BEGINNING:
                if (m_multiplier) {
                    m_funktable[0] = 9 * m_multiplier - 1;
                    m_funktable[1] = 6 * m_multiplier - 1;
                    chan.tempo     = 6 * m_multiplier - 1;
                }
                else {
                    m_funktable[0] = 9 - 1;
                    m_funktable[1] = 6 - 1;
                    chan.tempo     = 6 - 1;
                }
                if (song.instr[MAX_INSTR - 1].ad >= 2 && !song.instr[MAX_INSTR - 1].ptr[WTBL]) {
                    chan.tempo = song.instr[MAX_INSTR - 1].ad - 1;
                }
                chan.trans = 0;
                chan.instr = 1;
                sequencer(c);
                break;

            case PLAY_PATTERN:
                chan.advance = 0;
                chan.pattptr = m_startpattpos * 4;
                chan.pattnum = m_epnum[c];
                if (chan.pattptr >= song.pattlen[chan.pattnum] * 4) chan.pattptr = 0;
                break;

            case PLAY_POS:
                chan.songptr = m_espos[c];
                sequencer(c);
                break;

            default: break;
            }
        }
        if (m_songinit != PLAY_STOP) m_songinit = PLAY_FOO;
        else                         m_songinit = PLAY_STOPPED;

        if (song.songlen[m_psnum][0] == 0 ||
            song.songlen[m_psnum][1] == 0 ||
            song.songlen[m_psnum][2] == 0)
        {
            m_songinit = PLAY_STOPPED; // zero length song
        }

        m_startpattpos = 0;
        return;
    }

    if (m_filterptr) {
        // filter jump
        if (song.ltable[FTBL][m_filterptr - 1] == 0xff) {
            m_filterptr = song.rtable[FTBL][m_filterptr - 1];
            if (!m_filterptr) goto FILTERSTOP;
        }

        if (!m_filtertime) {
            // filter set
            if (song.ltable[FTBL][m_filterptr - 1] >= 0x80) {
                m_filtertype = song.ltable[FTBL][m_filterptr - 1] & 0x70;
                m_filterctrl = song.rtable[FTBL][m_filterptr - 1];
                m_filterptr++;
                // can be combined with cutoff set
                if (song.ltable[FTBL][m_filterptr - 1] == 0x00) {
                    m_filtercutoff = song.rtable[FTBL][m_filterptr - 1];
                    m_filterptr++;
                }
            }
            else {
                // new modulation step
                if (song.ltable[FTBL][m_filterptr - 1]) m_filtertime = song.ltable[FTBL][m_filterptr - 1];
                else {
                    // cutoff set
                    m_filtercutoff = song.rtable[FTBL][m_filterptr - 1];
                    m_filterptr++;
                }
            }
        }
        // filter modulation
        if (m_filtertime) {
            m_filtercutoff += song.rtable[FTBL][m_filterptr - 1];
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
        Channel&     chan = m_channels[c];
        Instr const& iptr = song.instr[chan.instr];

        // reset tempo in jammode
        if (m_songinit == PLAY_STOPPED && chan.tempo < 2) {
            if (m_multiplier) chan.tempo = 6 * m_multiplier - 1;
            else              chan.tempo = 6 - 1;
        }

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
        // advance in sequencer
        sequencer(c);

        // get gatetimer compare-value
        chan.gatetimer = iptr.gatetimer & 0x3f;

        // new note init
        if (chan.newnote) {
            chan.note     = chan.newnote - FIRSTNOTE;
            chan.command  = 0;
            chan.vibdelay = iptr.vibdelay;
            chan.cmddata  = iptr.ptr[STBL];
            if (chan.newcommand != CMD_TONEPORTA) {
                if (iptr.firstwave) {
                    if (iptr.firstwave >= 0xfe) chan.gate = iptr.firstwave;
                    else {
                        chan.wave = iptr.firstwave;
                        chan.gate = 0xff;
                    }
                }

                chan.ptr[WTBL] = iptr.ptr[WTBL];

                if (chan.ptr[WTBL]) {
                    // stop the song in case of jumping into a jump
                    if (song.ltable[WTBL][chan.ptr[WTBL] - 1] == 0xff) stop_song();
                }
                if (iptr.ptr[PTBL]) {
                    chan.ptr[PTBL] = iptr.ptr[PTBL];
                    chan.pulsetime = 0;
                    if (chan.ptr[PTBL]) {
                        // stop the song in case of jumping into a jump
                        if (song.ltable[PTBL][chan.ptr[PTBL] - 1] == 0xff) stop_song();
                    }
                }
                if (iptr.ptr[FTBL]) {
                    m_filterptr  = iptr.ptr[FTBL];
                    m_filtertime = 0;
                    if (m_filterptr) {
                        // stop the song in case of jumping into a jump
                        if (song.ltable[FTBL][m_filterptr - 1] == 0xff) stop_song();
                    }
                }
                regs[0x5 + 7 * c] = iptr.ad;
                regs[0x6 + 7 * c] = iptr.sr;
            }
        }

        // tick 0 effects

        switch (chan.newcommand) {
        case CMD_DONOTHING:
            chan.command = 0;
            chan.cmddata = iptr.ptr[STBL];
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
                if (song.ltable[WTBL][chan.ptr[WTBL] - 1] == 0xff) stop_song();
            }
            break;

        case CMD_SETPULSEPTR:
            chan.ptr[PTBL] = chan.newcmddata;
            chan.pulsetime = 0;
            if (chan.ptr[PTBL]) {
                // stop the song in case of jumping into a jump
                if (song.ltable[PTBL][chan.ptr[PTBL] - 1] == 0xff) stop_song();
            }
            break;

        case CMD_SETFILTERPTR:
            m_filterptr  = chan.newcmddata;
            m_filtertime = 0;
            if (m_filterptr) {
                // stop the song in case of jumping into a jump
                if (song.ltable[FTBL][m_filterptr - 1] == 0xff) stop_song();
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
                m_funktable[0] = song.ltable[STBL][chan.newcmddata - 1] - 1;
                m_funktable[1] = song.rtable[STBL][chan.newcmddata - 1] - 1;
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
            uint8_t wave = song.ltable[WTBL][chan.ptr[WTBL] - 1];
            uint8_t note = song.rtable[WTBL][chan.ptr[WTBL] - 1];

            if (wave > WAVELASTDELAY) {
                // normal waveform values
                if (wave < WAVESILENT) chan.wave = wave;
                // values without waveform selected
                if ((wave >= WAVESILENT) && (wave <= WAVELASTSILENT)) chan.wave = wave & 0xf;
                // command execution from wavetable
                if ((wave >= WAVECMD) && (wave <= WAVELASTCMD)) {
                    uint8_t param = song.rtable[WTBL][chan.ptr[WTBL] - 1];
                    switch (wave & 0xf) {
                    case CMD_DONOTHING:
                    case CMD_SETWAVEPTR:
                    case CMD_FUNKTEMPO: stop_song(); break;

                    case CMD_PORTAUP: {
                        uint16_t speed = 0;
                        if (param) {
                            speed = (song.ltable[STBL][param - 1] << 8) | song.rtable[STBL][param - 1];
                        }
                        if (speed >= 0x8000) {
                            speed = FREQ_LO[chan.lastnote + 1] | (FREQ_HI[chan.lastnote + 1] << 8);
                            speed -= FREQ_LO[chan.lastnote] | (FREQ_HI[chan.lastnote] << 8);
                            speed >>= song.rtable[STBL][param - 1];
                        }
                        chan.freq += speed;
                    } break;

                    case CMD_PORTADOWN: {
                        uint16_t speed = 0;
                        if (param) {
                            speed = (song.ltable[STBL][param - 1] << 8) | song.rtable[STBL][param - 1];
                        }
                        if (speed >= 0x8000) {
                            speed = FREQ_LO[chan.lastnote + 1] | (FREQ_HI[chan.lastnote + 1] << 8);
                            speed -= FREQ_LO[chan.lastnote] | (FREQ_HI[chan.lastnote] << 8);
                            speed >>= song.rtable[STBL][param - 1];
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
                            speed = (song.ltable[STBL][param - 1] << 8) | song.rtable[STBL][param - 1];
                            if (speed >= 0x8000) {
                                speed = FREQ_LO[chan.lastnote + 1] | (FREQ_HI[chan.lastnote + 1] << 8);
                                speed -= FREQ_LO[chan.lastnote] | (FREQ_HI[chan.lastnote] << 8);
                                speed >>= song.rtable[STBL][param - 1];
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
                            cmpvalue = song.ltable[STBL][param - 1];
                            speed    = song.rtable[STBL][param - 1];
                        }
                        if (cmpvalue >= 0x80) {
                            cmpvalue &= 0x7f;
                            speed = FREQ_LO[chan.lastnote + 1] | (FREQ_HI[chan.lastnote + 1] << 8);
                            speed -= FREQ_LO[chan.lastnote] | (FREQ_HI[chan.lastnote] << 8);
                            speed >>= song.rtable[STBL][param - 1];
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
                            if (song.ltable[PTBL][chan.ptr[PTBL] - 1] == 0xff) stop_song();
                        }
                        break;

                    case CMD_SETFILTERPTR:
                        m_filterptr  = param;
                        m_filtertime = 0;
                        if (m_filterptr) {
                            // stop the song in case of jumping into a jump
                            if (song.ltable[FTBL][m_filterptr - 1] == 0xff) stop_song();
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
            if (song.ltable[WTBL][chan.ptr[WTBL] - 1] == 0xff) {
                chan.ptr[WTBL] = song.rtable[WTBL][chan.ptr[WTBL] - 1];
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
        if (!m_optimizerealtime || chan.tick) {
            switch (chan.command) {
            case CMD_PORTAUP: {
                uint16_t speed = 0;
                if (chan.cmddata) {
                    speed = (song.ltable[STBL][chan.cmddata - 1] << 8) | song.rtable[STBL][chan.cmddata - 1];
                }
                if (speed >= 0x8000) {
                    speed = FREQ_LO[chan.lastnote + 1] | (FREQ_HI[chan.lastnote + 1] << 8);
                    speed -= FREQ_LO[chan.lastnote] | (FREQ_HI[chan.lastnote] << 8);
                    speed >>= song.rtable[STBL][chan.cmddata - 1];
                }
                chan.freq += speed;
            } break;

            case CMD_PORTADOWN: {
                uint16_t speed = 0;
                if (chan.cmddata) {
                    speed = (song.ltable[STBL][chan.cmddata - 1] << 8) | song.rtable[STBL][chan.cmddata - 1];
                }
                if (speed >= 0x8000) {
                    speed = FREQ_LO[chan.lastnote + 1] | (FREQ_HI[chan.lastnote + 1] << 8);
                    speed -= FREQ_LO[chan.lastnote] | (FREQ_HI[chan.lastnote] << 8);
                    speed >>= song.rtable[STBL][chan.cmddata - 1];
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
                    cmpvalue = song.ltable[STBL][chan.cmddata - 1];
                    speed    = song.rtable[STBL][chan.cmddata - 1];
                }
                if (cmpvalue >= 0x80) {
                    cmpvalue &= 0x7f;
                    speed = FREQ_LO[chan.lastnote + 1] | (FREQ_HI[chan.lastnote + 1] << 8);
                    speed -= FREQ_LO[chan.lastnote] | (FREQ_HI[chan.lastnote] << 8);
                    speed >>= song.rtable[STBL][chan.cmddata - 1];
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
                    speed = (song.ltable[STBL][chan.cmddata - 1] << 8) | song.rtable[STBL][chan.cmddata - 1];
                    if (speed >= 0x8000) {
                        speed = FREQ_LO[chan.lastnote + 1] | (FREQ_HI[chan.lastnote + 1] << 8);
                        speed -= FREQ_LO[chan.lastnote] | (FREQ_HI[chan.lastnote] << 8);
                        speed >>= song.rtable[STBL][chan.cmddata - 1];
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
            if (m_songinit != PLAY_STOPPED && chan.tick == chan.gatetimer) goto GETNEWNOTES;
        }

        if (chan.ptr[PTBL]) {
            // skip pulse when sequencer has been executed
            if (m_optimizepulse) {
                if (!chan.tick && !chan.pattptr) goto NEXTCHN;
            }

            // pulsetable jump
            if (song.ltable[PTBL][chan.ptr[PTBL] - 1] == 0xff) {
                chan.ptr[PTBL] = song.rtable[PTBL][chan.ptr[PTBL] - 1];
                if (!chan.ptr[PTBL]) goto PULSEEXEC;
            }

            if (!chan.pulsetime) {
                // set pulse
                if (song.ltable[PTBL][chan.ptr[PTBL] - 1] >= 0x80) {
                    chan.pulse = (song.ltable[PTBL][chan.ptr[PTBL] - 1] & 0xf) << 8;
                    chan.pulse |= song.rtable[PTBL][chan.ptr[PTBL] - 1];
                    chan.ptr[PTBL]++;
                }
                else {
                    chan.pulsetime = song.ltable[PTBL][chan.ptr[PTBL] - 1];
                }
            }
            // pulse modulation
            if (chan.pulsetime) {
                uint8_t speed = song.rtable[PTBL][chan.ptr[PTBL] - 1];
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
        if (m_songinit == PLAY_STOPPED || chan.tick != chan.gatetimer) goto NEXTCHN;

GETNEWNOTES:
        // new notes processing
        {
            uint8_t newnote = song.pattern[chan.pattnum][chan.pattptr];
            if (song.pattern[chan.pattnum][chan.pattptr + 1]) {
                chan.instr = song.pattern[chan.pattnum][chan.pattptr + 1];
            }
            chan.newcommand = song.pattern[chan.pattnum][chan.pattptr + 2];
            chan.newcmddata = song.pattern[chan.pattnum][chan.pattptr + 3];
            chan.pattptr += 4;
            if (song.pattern[chan.pattnum][chan.pattptr] == ENDPATT) chan.pattptr = 0x7fffffff;

            if (newnote == KEYOFF) chan.gate = 0xfe;
            if (newnote == KEYON) chan.gate = 0xff;
            if (newnote <= LASTNOTE) {
                chan.newnote = newnote + chan.trans;
                if ((chan.newcommand) != CMD_TONEPORTA) {
                    if (!(song.instr[chan.instr].gatetimer & 0x40)) {
                        chan.gate = 0xfe;
                        if (!(song.instr[chan.instr].gatetimer & 0x80)) {
                            regs[0x5 + 7 * c] = m_adparam >> 8;
                            regs[0x6 + 7 * c] = m_adparam & 0xff;
                        }
                    }
                }
            }
        }
NEXTCHN:
        // 2bt: also set freq on muted channel for sync/ringmod
        regs[0x0 + 7 * c] = chan.freq & 0xff;
        regs[0x1 + 7 * c] = chan.freq >> 8;
        regs[0x2 + 7 * c] = chan.pulse & 0xfe;
        regs[0x3 + 7 * c] = chan.pulse >> 8;
        if (chan.mute) {
            regs[0x4 + 7 * c] = chan.wave & 0x08; // 2bt: don't set test bit every time
        }
        else {
            regs[0x4 + 7 * c] = chan.wave & chan.gate;
        }
    }
    //if (songinit != PLAY_STOPPED) incrementtime();
}

} // namespace gt
