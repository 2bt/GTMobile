#pragma once
#include <cstdint>
#include <memory>


class SID;


class SidEngine {
public:
    SidEngine(int mixrate);
    ~SidEngine();
    void write(int reg, uint8_t value);
    void mix(int16_t* buffer, int length);
private:
    std::unique_ptr<SID> m_sid;
};
