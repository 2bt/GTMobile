#pragma once
#include <cstdint>
#include <memory>
#include <array>


class Sid {
public:
    enum {
        MIXRATE        = 44100,
        CLOCKRATE_PAL  = 985248,
        CLOCKRATE_NTSC = 1022727,
    };
    enum class Model { MOS6581, MOS8580 };
    enum class SamplingMethod {
        Fast,
        Interpolate,
        ResampleInterpolate,
        ResampleFast,
    };

    Sid();
    ~Sid();
    Sid(Sid&&) noexcept;
    Sid& operator=(Sid&&) noexcept;
    void                 init(Model model, SamplingMethod sampling_method);
    void                 reset();
    void                 set_chip_model(Model model);
    void                 set_sampling_method(SamplingMethod sampling_method);
    void                 set_reg(int reg, uint8_t value);
    int                  clock(int cycles, int16_t* buffer, int length);
    std::array<float, 3> get_env_levels();
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
