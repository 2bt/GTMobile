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


    void box(Box const& box, int style = 0) {
        if (style == 0) {
            i16vec2 uv(1, 65);
            auto i0 = add_vertex({ box.pos, uv, m_color });
            auto i1 = add_vertex({ box.pos + box.size.xo(), uv, m_color });
            auto i2 = add_vertex({ box.pos + box.size, uv, m_color });
            auto i3 = add_vertex({ box.pos + box.size.oy(), uv, m_color });
            m_indices.insert(m_indices.end(), { i0, i1, i2, i0, i2, i3 });
        }

        int s = 8;
        int o = 64;
        if (box.size.x < 16 || box.size.y < 16) {
            s = 4;
            o += 16;
        }
        if (box.size.x < 8 || box.size.y < 8) {
            s = 1;
            o += 16;
        }

        i16vec2 p0 = box.pos;
        i16vec2 p1 = box.pos + ivec2(s);
        i16vec2 p2 = box.pos + box.size - ivec2(s);
        i16vec2 p3 = box.pos + box.size;
        i16vec2 t0 = i16vec2(16 * style, o);
        i16vec2 t1 = t0 + ivec2(s);
        i16vec2 t2 = t1;
        i16vec2 t3 = t2 + ivec2(s);
        auto i0  = add_vertex({ { p0.x, p0.y }, { t0.x, t0.y }, m_color });
        auto i1  = add_vertex({ { p1.x, p0.y }, { t1.x, t0.y }, m_color });
        auto i2  = add_vertex({ { p2.x, p0.y }, { t2.x, t0.y }, m_color });
        auto i3  = add_vertex({ { p3.x, p0.y }, { t3.x, t0.y }, m_color });
        auto i4  = add_vertex({ { p0.x, p1.y }, { t0.x, t1.y }, m_color });
        auto i5  = add_vertex({ { p1.x, p1.y }, { t1.x, t1.y }, m_color });
        auto i6  = add_vertex({ { p2.x, p1.y }, { t2.x, t1.y }, m_color });
        auto i7  = add_vertex({ { p3.x, p1.y }, { t3.x, t1.y }, m_color });
        auto i8  = add_vertex({ { p0.x, p2.y }, { t0.x, t2.y }, m_color });
        auto i9  = add_vertex({ { p1.x, p2.y }, { t1.x, t2.y }, m_color });
        auto i10 = add_vertex({ { p2.x, p2.y }, { t2.x, t2.y }, m_color });
        auto i11 = add_vertex({ { p3.x, p2.y }, { t3.x, t2.y }, m_color });
        auto i12 = add_vertex({ { p0.x, p3.y }, { t0.x, t3.y }, m_color });
        auto i13 = add_vertex({ { p1.x, p3.y }, { t1.x, t3.y }, m_color });
        auto i14 = add_vertex({ { p2.x, p3.y }, { t2.x, t3.y }, m_color });
        auto i15 = add_vertex({ { p3.x, p3.y }, { t3.x, t3.y }, m_color });

        m_indices.insert(m_indices.end(), {
            i0,  i1,  i5,  i0,  i5,  i4,
            i1,  i2,  i6,  i1,  i6,  i5,
            i2,  i3,  i7,  i2,  i7,  i6,
            i4,  i5,  i9,  i4,  i9,  i8,
            i5,  i6,  i10, i5,  i10, i9,
            i6,  i7,  i11, i6,  i11, i10,
            i8,  i9,  i13, i8,  i13, i12,
            i9,  i10, i14, i9,  i14, i13,
            i10, i11, i15, i10, i15, i14,
        });
    }



private:
    u8vec4 m_color        = {255};
    int    m_font_offset  = 192; // 0;
    ivec2  m_char_size    = {8, 8};

};


void set_refresh_rate(float refresh_rate);


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


bool button(Icon icon, bool active);

void touch(int x, int y, bool pressed);


} // namespace
