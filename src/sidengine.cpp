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

#include "sidengine.hpp"

enum {
    PALCLOCKRATE = 985248,
};

SidEngine::SidEngine() {
    m_sid = std::make_unique<SID>();
    m_sid->reset();
    m_sid->set_chip_model(MOS8580);
    m_sid->set_sampling_parameters(PALCLOCKRATE, SAMPLE_RESAMPLE_INTERPOLATE, MIXRATE);
}

SidEngine::~SidEngine() {}

void SidEngine::write(int reg, uint8_t value) {
    m_sid->write(reg, value);
}

void SidEngine::mix(int16_t* buffer, int length) {
    int c = 999999999;
    m_sid->clock(c, buffer, length);
}
