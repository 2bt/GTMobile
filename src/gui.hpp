#pragma once
#include "gfx.hpp"


namespace gui {

constexpr u8vec4 rgb(uint32_t c, uint8_t a = 255) {
    return { uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c), a };
}

constexpr u8vec4 BLACK       = rgb(0x000000);
constexpr u8vec4 DARK_BLUE   = rgb(0x1D2B53);
constexpr u8vec4 DARK_PURPLE = rgb(0x7E2553);
constexpr u8vec4 DARK_GREEN  = rgb(0x008751);
constexpr u8vec4 BROWN       = rgb(0xAB5236);
constexpr u8vec4 DARK_GREY   = rgb(0x5F574F);
constexpr u8vec4 LIGHT_GREY  = rgb(0xC2C3C7);
constexpr u8vec4 WHITE       = rgb(0xFFF1E8);
constexpr u8vec4 RED         = rgb(0xFF004D);
constexpr u8vec4 ORANGE      = rgb(0xFFA300);
constexpr u8vec4 YELLOW      = rgb(0xFFEC27);
constexpr u8vec4 GREEN       = rgb(0x00E436);
constexpr u8vec4 BLUE        = rgb(0x29ADFF);
constexpr u8vec4 LAVENDER    = rgb(0x83769C);
constexpr u8vec4 PINK        = rgb(0xFF77A8);
constexpr u8vec4 LIGHT_PEACH = rgb(0xFFCCAA);



struct Box {
    bool contains(ivec2 p) const {
        return p.x >= pos.x && p.y >= pos.y &&
               p.x < pos.x + size.x && p.y < pos.y + size.y;
    }
    ivec2 pos;
    ivec2 size;
};


enum class BoxStyle {
    Normal,
    Round,
    Tab,
    Frame,
    Round2,
    PianoKey,
};


class DrawContext : public gfx::DrawContext {
public:

    void character(ivec2 pos, uint8_t g) {
        ivec2 uv(g % 16 * m_char_size.x, g / 16 * m_char_size.x + m_font_offset);
        rect(pos, m_char_size, uv);
    }

    void text(ivec2 pos, const char* text) {
        ivec2 p = pos;
        while (uint8_t c = *text++) {
            if (c < 128) character(p, c);
            p.x += m_char_size.x;
        }
    }

    void fill(Box const& box);
    void box(Box const& box, BoxStyle style = BoxStyle::Normal);

private:
    int    m_font_offset  = 192; // 0;
    ivec2  m_char_size    = {8, 8};
};


enum class Icon {
    None = 63,
    Loop = 48,
    Stop,
    Play,
    FastBackward,
    FastForward,
    Follow,
    VGrab,
    HGrab,

    Copy = 56,
    Paste,

    Lowpass = 64,
    Bandpass,
    Highpass,

    Noise = 72,
    Pulse,
    Saw,
    Tri,
    Ring,
    Sync,
    VGate,
    HGate,

    AddRowAbove = 80,
    AddRowBelow,
    DeleteRow,
};


void init();
void free();
void set_refresh_rate(float refresh_rate);
void begin_frame();
void end_frame();


namespace touch {
    void event(int x, int y, bool pressed);
    bool pressed();
    bool just_pressed();
    bool just_released();
    bool touched(Box const& box);
    bool just_touched(Box const& box);
} // namespace touch


void id(void const* addr);
void cursor(ivec2 pos);
void item_size(ivec2 size);
void item_padding(ivec2 padding);
void same_line(bool same_line = true);
bool has_active_item();

void button_style(BoxStyle style);
bool button(Icon icon, bool active = false);

bool horizontal_drag_bar(int& value, int min, int max, int page);
bool vertical_drag_bar(int& value, int min, int max, int page);

bool vertical_drag_button(int& value);

DrawContext& get_draw_context();

} // namespace
