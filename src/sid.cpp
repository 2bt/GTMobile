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


struct Sid::Impl {
    SID sid;
};

Sid::Sid() = default;
Sid::~Sid() = default;
Sid::Sid(Sid&&) noexcept = default;
Sid& Sid::operator=(Sid&&) noexcept = default;


void Sid::init(Model model, SamplingMethod sampling_method) {
    if (!impl) impl = std::make_unique<Impl>();
    impl->sid.reset();
    impl->sid.set_chip_model(model == Model::MOS6581 ? MOS6581 : MOS8580);
    impl->sid.set_sampling_parameters(
            CLOCKRATE_PAL,
            sampling_method == SamplingMethod::Fast ? SAMPLE_FAST : SAMPLE_RESAMPLE_INTERPOLATE,
            MIXRATE);
}

void Sid::set_reg(int reg, uint8_t value) {
    impl->sid.write(reg, value);
}

int Sid::clock(int cycles, int16_t* buffer, int length) {
    return impl->sid.clock(cycles, buffer, length);
}

std::array<float, 3> Sid::get_env_levels() {
    SID::State state = impl->sid.read_state();
    std::array<float, 3> levels = {};
    for (int c = 0; c < 3; ++c) {
        if (state.sid_register[c * 7 + 4] & 0xf0) levels[c] = state.envelope_counter[c] * (1.0f / 0xff);
    }
    return levels;
}
