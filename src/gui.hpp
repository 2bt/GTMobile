#pragma once
#include "gfx.hpp"


namespace color {

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
    constexpr u8vec4 WHITE       = rgb(0xffffff);
    constexpr u8vec4 RED         = rgb(0xFF004D);
    constexpr u8vec4 ORANGE      = rgb(0xFFA300);
    constexpr u8vec4 YELLOW      = rgb(0xFFEC27);
    constexpr u8vec4 GREEN       = rgb(0x00E436);
    constexpr u8vec4 BLUE        = rgb(0x29ADFF);
    constexpr u8vec4 LAVENDER    = rgb(0x83769C);
    constexpr u8vec4 PINK        = rgb(0xFF77A8);
    constexpr u8vec4 LIGHT_PEACH = rgb(0xFFCCAA);


    constexpr u8vec4 DRAG_BG            = rgb(0x222222);
    constexpr u8vec4 DRAG_HANDLE_NORMAL = rgb(0x444444);
    constexpr u8vec4 DRAG_HANDLE_ACTIVE = rgb(0x666666);
    constexpr u8vec4 DRAG_ICON          = rgb(0x777777);

    constexpr u8vec4 BUTTON_NORMAL      = DARK_GREY;
    constexpr u8vec4 BUTTON_ACTIVE      = mix(color::ORANGE, color::BLACK, 0.3f);
    constexpr u8vec4 BUTTON_PRESSED     = mix(color::ORANGE, color::YELLOW, 0.3f);
    constexpr u8vec4 BUTTON_HELD        = mix(color::ORANGE, color::YELLOW, 0.3f);
    constexpr u8vec4 BUTTON_RELEASED    = color::YELLOW;

    constexpr u8vec4 CMDS[16] = {
        mix(color::rgb(0x000000), color::WHITE, 0.2f),

        mix(color::rgb(0xff77a8), color::WHITE, 0.2f), // 1 portamento up
        mix(color::rgb(0xff77a8), color::WHITE, 0.2f), // 2 portamento down
        mix(color::rgb(0xff77a8), color::WHITE, 0.2f), // 3 tone portamento

        mix(color::rgb(0xff2030), color::WHITE, 0.2f), // 4 vibrato

        mix(color::rgb(0x00e436), color::WHITE, 0.2f), // 5 attack/decay
        mix(color::rgb(0x00e436), color::WHITE, 0.2f), // 6 sustain/release

        mix(color::rgb(0xffa300), color::WHITE, 0.2f), // 7 waveform reg
        mix(color::rgb(0xffa300), color::WHITE, 0.2f), // 8 wavetable ptr
        mix(color::rgb(0xffa300), color::WHITE, 0.2f), // 9 pulsetable ptr

        mix(color::rgb(0x29adff), color::WHITE, 0.2f), // A filtertable ptr
        mix(color::rgb(0x29adff), color::WHITE, 0.2f), // B filter control
        mix(color::rgb(0x29adff), color::WHITE, 0.2f), // C filter cutoff

        mix(color::rgb(0x00e436), color::WHITE, 0.2f), // D master volume

        mix(color::rgb(0xffec27), color::WHITE, 0.2f), // E funk tempo
        mix(color::rgb(0xffec27), color::WHITE, 0.2f), // F tempo

    };

    constexpr u8vec4 ROW_NUMBER     = color::rgb(0xaaaaaa);
    constexpr u8vec4 INSTRUMENT     = color::rgb(0xaabbdd);
    constexpr u8vec4 HIGHLIGHT_ROW  = color::rgb(0x333333);
    constexpr u8vec4 BACKGROUND_ROW = color::rgb(0x171717);
    constexpr u8vec4 PLAYER_ROW     = color::rgb(0x553311);



} // namespace color


namespace gui {


struct Box {
    bool contains(ivec2 p) const {
        return p.x >= pos.x && p.y >= pos.y &&
               p.x < pos.x + size.x && p.y < pos.y + size.y;
    }
    ivec2 pos;
    ivec2 size;
};


enum class BoxStyle {
    Fill,
    Normal,
    Tab,
    Text,
    Cursor,
    Window,

    PianoKey = 8,
};


class DrawContext {
public:

    void fill(Box const& box) {
        i16vec2 uv(8, 8); // a white pixel
        auto i0 = add_vertex({ box.pos, uv, m_color });
        auto i1 = add_vertex({ box.pos + box.size.xo(), uv, m_color });
        auto i2 = add_vertex({ box.pos + box.size, uv, m_color });
        auto i3 = add_vertex({ box.pos + box.size.oy(), uv, m_color });
        m_mesh->indices.insert(m_mesh->indices.end(), { i0, i1, i2, i0, i2, i3 });
    }

    void rect(ivec2 pos, ivec2 size, ivec2 uv) {
        uint16_t i0 = add_vertex({ pos, uv, m_color });
        uint16_t i1 = add_vertex({ pos + ivec2(size.x, 0), uv + ivec2(size.x, 0), m_color });
        uint16_t i2 = add_vertex({ pos + size, uv + size, m_color });
        uint16_t i3 = add_vertex({ pos + ivec2(0, size.y), uv + ivec2(0, size.y), m_color });
        m_mesh->indices.insert(m_mesh->indices.end(), { i0, i1, i2, i0, i2, i3 });
    }

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

    void box(Box const& box, BoxStyle style = BoxStyle::Normal);

    void mesh(gfx::Mesh& mesh) {
        m_mesh = &mesh;
    }
    void color(u8vec4 color) {
        m_color = color;
    }
    void font(int i) {
        m_font_offset = 256 + 64 * i;
    }

private:

    uint16_t add_vertex(gfx::Vertex v) {
        uint16_t index = m_mesh->vertices.size();
        m_mesh->vertices.emplace_back(v);
        return index;
    }

    u8vec4     m_color       = {255};
    int        m_font_offset = 256;
    ivec2      m_char_size   = {8, 8};
    gfx::Mesh* m_mesh;

};


enum class Icon {
    Left = 16,
    Right,
    Up,
    Down,
    VGrab,
    HGrab,
    Off,
    On,

    AddRowAbove = 24,
    AddRowBelow,
    DeleteRow,
    Pen,
    Trash,
    Settings,

    Loop = 40,
    Stop,
    Play,
    FastBackward,
    FastForward,
    Follow,
    Record,

    Copy = 48,
    Paste,

    Noise = 56,
    Pulse,
    Saw,
    Triangle,
    Test,
    Ring,
    Sync,
    Gate,

    Lowpass = 64,
    Bandpass,
    Highpass,
};

enum class Align { Left, Center };

enum class ButtonStyle {
    Normal,
    Tab,
    TableCell,
};
enum class DragBarStyle {
    Normal,
    Scrollbar,
};

enum {
    KEYCODE_ENTER = 66,
    KEYCODE_DEL   = 67,
};


void init();
void free();
void touch_event(int x, int y, bool pressed);
void key_event(int key, int unicode);
void set_refresh_rate(float refresh_rate);


namespace touch {
    bool pressed();
    bool just_pressed();
    bool just_released();
    bool touched(Box const& box);
    bool just_touched(Box const& box);
} // namespace touch


void begin_frame();
void end_frame();

void begin_window();
Box  begin_window(ivec2 size);
void end_window();

void id(void const* addr);
void cursor(ivec2 pos);
ivec2 cursor();
void item_size(ivec2 size);
void same_line(bool same_line = true);
bool has_active_item();
void align(Align a);
void button_style(ButtonStyle style);
void drag_bar_style(DragBarStyle style);

bool hold();
void text(char const* fmt, ...);
bool button(Icon icon, bool active = false);
bool button(char const* label, bool active = false);
void input_text(char* str, int len);
template<class T>void input_text(T& t) { input_text(t.data(), t.size() - 1); }
bool horizontal_drag_bar(int& value, int min, int max, int page);
bool vertical_drag_bar(int& value, int min, int max, int page);
bool vertical_drag_button(int& value);

template<class T>
bool horizontal_drag_bar(T& value, int min, int max, int page = 0) {
    int v = value;
    id(&value);
    bool b = horizontal_drag_bar(v, min, max, page);
    if (b) value = v;
    return b;
}


// low level functions
enum class ButtonState { Normal, Pressed, Held, Released };
DrawContext& draw_context();
Box          item_box();
ButtonState  button_state(Box const& box, void const* addr = nullptr);

} // namespace
