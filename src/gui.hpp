#pragma once
#include "gfx.hpp"


namespace gui {

constexpr u8vec4 color(uint32_t c, uint8_t a = 255) {
    return { uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c), a };
}

constexpr u8vec4 BLACK       = color(0x000000);
constexpr u8vec4 DARK_BLUE   = color(0x1D2B53);
constexpr u8vec4 DARK_PURPLE = color(0x7E2553);
constexpr u8vec4 DARK_GREEN  = color(0x008751);
constexpr u8vec4 BROWN       = color(0xAB5236);
constexpr u8vec4 DARK_GREY   = color(0x5F574F);
constexpr u8vec4 LIGHT_GREY  = color(0xC2C3C7);
constexpr u8vec4 WHITE       = color(0xFFF1E8);
constexpr u8vec4 RED         = color(0xFF004D);
constexpr u8vec4 ORANGE      = color(0xFFA300);
constexpr u8vec4 YELLOW      = color(0xFFEC27);
constexpr u8vec4 GREEN       = color(0x00E436);
constexpr u8vec4 BLUE        = color(0x29ADFF);
constexpr u8vec4 LAVENDER    = color(0x83769C);
constexpr u8vec4 PINK        = color(0xFF77A8);
constexpr u8vec4 LIGHT_PEACH = color(0xFFCCAA);



struct Box {
    bool contains(ivec2 p) const {
        return p.x >= pos.x && p.y >= pos.y &&
               p.x < pos.x + size.x && p.y < pos.y + size.y;
    }
    ivec2 pos;
    ivec2 size;
};


class DrawContext : public gfx::DrawContext {
public:

    void set_color(u8vec4 color) { m_color = color; }

    void character(ivec2 pos, uint8_t g) {
        ivec2 uv(g % 16 * m_char_size.x, g / 16 * m_char_size.x + m_font_offset);
        rect(pos, m_char_size, uv, m_color);
    }

    void text(ivec2 pos, const char* text) {
        ivec2 p = pos;
        while (uint8_t c = *text++) {
            if (c < 128) character(p, c);
            p.x += m_char_size.x;
        }
    }

    void box(Box const& box, int style = 0);


private:
    u8vec4 m_color        = {255};
    int    m_font_offset  = 192; // 0;
    ivec2  m_char_size    = {8, 8};

};




enum class Icon {
    LOOP = 48,
    STOP,
    PLAY,
    FAST_BACKWARD,
    FAST_FORWARD,
    TOUCH,

    COPY = 56,
    PASTE,

    LOWPASS = 64,
    BANDPASS,
    HIGHPASS,

    NOISE = 72,
    PULSE,
    SAW,
    TRI,
    RING,
    SYNC,
    GATE,

    ADD_ROW_ABOVE = 80,
    ADD_ROW_BELOW,
    DELETE_ROW,
};


void init();
void free();
void set_refresh_rate(float refresh_rate);
void touch(int x, int y, bool pressed);
void end_frame();
DrawContext& get_draw_context();

bool button(Icon icon, bool active);


} // namespace
