#include "resid/envelope.cpp"
#include "resid/extfilt.cpp"
#include "resid/filter.cpp"
#include "resid/pot.cpp"
#include "resid/sid.cpp"
#include "resid/version.cpp"
#include "resid/voice.cpp"
#include "resid/wave6581_PS_.cpp"
#include "resid/wave6581_PST.cpp"
#include "resid/wave6581_P_T.cpp"
#include "resid/wave6581__ST.cpp"
#include "resid/wave8580_PS_.cpp"
#include "resid/wave8580_PST.cpp"
#include "resid/wave8580_P_T.cpp"
#include "resid/wave8580__ST.cpp"
#include "resid/wave.cpp"

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
    reset();
    set_chip_model(model);
    set_sampling_method(sampling_method);
    impl->sid.clock(10000); // clock some cycles to surpress the initial clicking
}
void Sid::reset() {
    impl->sid.reset();
}
void Sid::set_chip_model(Model model) {
    impl->sid.set_chip_model(model == Model::MOS6581 ? MOS6581 : MOS8580);
}
void Sid::set_sampling_method(SamplingMethod sampling_method) {
    impl->sid.set_sampling_parameters(CLOCKRATE_PAL, ::sampling_method(sampling_method), MIXRATE);
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
