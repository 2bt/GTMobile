#include "resid/envelope.cc"
#include "resid/extfilt.cc"
#include "resid/filter.cc"
#include "resid/pot.cc"
#include "resid/sid.cc"
#include "resid/version.cc"
#include "resid/voice.cc"
#include "resid/wave6581_PS_.cc"
#include "resid/wave6581_PST.cc"
#include "resid/wave6581_P_T.cc"
#include "resid/wave6581__ST.cc"
#include "resid/wave8580_PS_.cc"
#include "resid/wave8580_PST.cc"
#include "resid/wave8580_P_T.cc"
#include "resid/wave8580__ST.cc"
#include "resid/wave.cc"

#include "sid.hpp"

namespace sid {
namespace {
    enum {
        PALCLOCKRATE = 985248,
        MIXRATE      = 44100,

    };
    SID        g_sid;
    SID::State g_state;
} // namespace


void init() {
    g_sid.reset();
    g_sid.set_chip_model(MOS8580);
    // g_sid.set_sampling_parameters(PALCLOCKRATE, SAMPLE_RESAMPLE_INTERPOLATE, mixrate);
    g_sid.set_sampling_parameters(PALCLOCKRATE, SAMPLE_FAST, MIXRATE);
}

void write(int reg, uint8_t value) {
    g_sid.write(reg, value);
}

void mix(int16_t* buffer, int length) {
    int c = 999999999;
    g_sid.clock(c, buffer, length);
}

void update_state() {
    g_state = g_sid.read_state();
}

float chan_level(int c) {
    if (!(g_state.sid_register[c * 7 + 4] & 0xf0)) return 0;
    return g_state.envelope_counter[c] * (1.0f / 0xff);
}

} // namespace sid
