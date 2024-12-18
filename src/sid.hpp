#pragma once
#include <cstdint>
#include <memory>
#include <array>


class Sid {
public:
    enum {
        MIXRATE       = 44100,
        PALCLOCKRATE  = 985248,
        NTSCCLOCKRATE = 1022727,
    };
    enum class Model { MOS6581, MOS8580 };
    enum class SamplingMethod { Fast, Good };

    Sid();
    ~Sid();
    Sid(Sid&&) noexcept;
    Sid& operator=(Sid&&) noexcept;
    Sid(Model model, int clock_rate, SamplingMethod sampling_method);
    void                 set_reg(int reg, uint8_t value);
    int                  clock(int cycles, int16_t* buffer, int length);
    std::array<float, 3> get_env_levels();
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
