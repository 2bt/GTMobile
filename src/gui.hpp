#pragma once
#include "gfx.hpp"


namespace color {

    constexpr uint32_t mix(uint32_t c1, uint32_t c2, float x) {
        float y = 1.0f - x;
        return uint8_t(uint8_t(c1 >> 16) * y + uint8_t(c2 >> 16) * x) << 16 |
               uint8_t(uint8_t(c1 >> 8) * y + uint8_t(c2 >> 8) * x) << 8 |
               uint8_t(uint8_t(c1) * y + uint8_t(c2) * x);
    }

    constexpr uint32_t BLACK       = 0x000000;
    constexpr uint32_t DARK_BLUE   = 0x1D2B53;
    constexpr uint32_t DARK_PURPLE = 0x7E2553;
    constexpr uint32_t DARK_GREEN  = 0x008751;
    constexpr uint32_t BROWN       = 0xAB5236;
    constexpr uint32_t DARK_GREY   = 0x5F574F;
    constexpr uint32_t LIGHT_GREY  = 0xC2C3C7;
    constexpr uint32_t WHITE       = 0xffffff;
    constexpr uint32_t RED         = 0xFF004D;
    constexpr uint32_t ORANGE      = 0xFFA300;
    constexpr uint32_t YELLOW      = 0xFFEC27;
    constexpr uint32_t GREEN       = 0x00E436;
    constexpr uint32_t BLUE        = 0x29ADFF;
    constexpr uint32_t LAVENDER    = 0x83769C;
    constexpr uint32_t PINK        = 0xFF77A8;
    constexpr uint32_t LIGHT_PEACH = 0xFFCCAA;


    constexpr uint32_t DRAG_BG            = 0x222222;
    constexpr uint32_t DRAG_HANDLE_NORMAL = 0x444444;
    constexpr uint32_t DRAG_HANDLE_ACTIVE = 0x666666;
    constexpr uint32_t DRAG_ICON          = 0x777777;

    constexpr uint32_t BUTTON_NORMAL      = DARK_GREY;
    constexpr uint32_t BUTTON_ACTIVE      = mix(ORANGE, BLACK, 0.3f);
    constexpr uint32_t BUTTON_PRESSED     = mix(ORANGE, YELLOW, 0.3f);
    constexpr uint32_t BUTTON_HELD        = mix(ORANGE, YELLOW, 0.3f);
    constexpr uint32_t BUTTON_RELEASED    = YELLOW;

    constexpr uint32_t CMDS[16] = {
        mix(0x000000, WHITE, 0.2f),

        mix(0xff77a8, WHITE, 0.2f), // 1 portamento up
        mix(0xff77a8, WHITE, 0.2f), // 2 portamento down
        mix(0xff77a8, WHITE, 0.2f), // 3 tone portamento

        mix(0xff2030, WHITE, 0.2f), // 4 vibrato

        mix(0x00e436, WHITE, 0.2f), // 5 attack/decay
        mix(0x00e436, WHITE, 0.2f), // 6 sustain/release

        mix(0xffa300, WHITE, 0.2f), // 7 waveform reg
        mix(0xffa300, WHITE, 0.2f), // 8 wavetable ptr
        mix(0xffa300, WHITE, 0.2f), // 9 pulsetable ptr

        mix(0x29adff, WHITE, 0.2f), // A filtertable ptr
        mix(0x29adff, WHITE, 0.2f), // B filter control
        mix(0x29adff, WHITE, 0.2f), // C filter cutoff

        mix(0x00e436, WHITE, 0.2f), // D master volume

        mix(0xffec27, WHITE, 0.2f), // E funk tempo
        mix(0xffec27, WHITE, 0.2f), // F tempo

    };

    constexpr uint32_t ROW_NUMBER     = 0xaaaaaa;
    constexpr uint32_t INSTRUMENT     = 0xaabbdd;
    constexpr uint32_t HIGHLIGHT_ROW  = 0x333333;
    constexpr uint32_t BACKGROUND_ROW = 0x171717;
    constexpr uint32_t PLAYER_ROW     = 0x553311;

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
    Shaded,
    Tab,
    ShadedTab,
    Text,
    Cursor,
    Window,
    PianoKey,
    RadioLeft,
    RadioCenter,
    RadioRight,
    Frame,
};


class DrawContext {
public:

    void fill(Box const& box) {
        i16vec2 uv(8, 8); // a white pixel
        auto i0 = add_vertex(box.pos, uv);
        auto i1 = add_vertex(box.pos + box.size.xo(), uv);
        auto i2 = add_vertex(box.pos + box.size, uv);
        auto i3 = add_vertex(box.pos + box.size.oy(), uv);
        m_mesh->indices.insert(m_mesh->indices.end(), { i0, i1, i2, i0, i2, i3 });
    }

    void rect(ivec2 pos, ivec2 size, ivec2 uv) {
        uint16_t i0 = add_vertex(pos, uv);
        uint16_t i1 = add_vertex(pos + ivec2(size.x, 0), uv + ivec2(size.x, 0));
        uint16_t i2 = add_vertex(pos + size, uv + size);
        uint16_t i3 = add_vertex(pos + ivec2(0, size.y), uv + ivec2(0, size.y));
        m_mesh->indices.insert(m_mesh->indices.end(), { i0, i1, i2, i0, i2, i3 });
    }

    void character(ivec2 pos, uint8_t g) {
        ivec2 uv(g % 16 * m_char_size.x, g / 16 * m_char_size.x + m_font_offset);
        rect(pos, m_char_size, uv);
    }

    ivec2 text_size(char const* text) {
        int y = 1;
        int x = 0;
        int mx = 0;
        while (uint8_t c = *text++) {
            if (c == '\n') {
                ++y;
                x = 0;
            }
            else {
                mx = std::max(mx, ++x);
            }
        }
        return {x * m_char_size.x, y * m_char_size.y};
    }
    void text(ivec2 pos, char const* text) {
        ivec2 p = pos;
        while (uint8_t c = *text++) {
            if (c < 128) character(p, c);
            if (c == '\n') {
                p.x = pos.x;
                p.y += m_char_size.y;
            }
            else {
                p.x += m_char_size.x;
            }
        }
    }

    void box(Box const& box, BoxStyle style = BoxStyle::Normal);

    void mesh(gfx::Mesh& mesh) {
        m_mesh = &mesh;
    }
    void rgb(uint32_t c) {
        m_color.x = uint8_t(c >> 16);
        m_color.y = uint8_t(c >> 8);
        m_color.z = uint8_t(c);
    }
    void alpha(uint8_t alpha) {
        m_color.w = alpha;
    }
    void font(int i) {
        m_font_offset = 256 + 64 * i;
    }

private:
    uint16_t add_vertex(i16vec2 pos, i16vec2 uv) {
        uint16_t index = m_mesh->vertices.size();
        m_mesh->vertices.push_back({ pos, uv, m_color });
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
    Sliders,
    JumpBack,

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

enum class Align { Left, Center, Right };

enum class ButtonStyle {
    Normal,
    Shaded,
    Tab,
    ShadedTab,
    TableCell,
    RadioLeft,
    RadioCenter,
    RadioRight,
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
float get_frame_time();


namespace touch {
    bool pressed();
    bool just_pressed();
    bool just_released();
    bool touched(Box const& box);
    bool just_touched(Box const& box);
} // namespace touch


void begin_frame();
void end_frame();

void disabled(bool disabled);
bool get_disabled();

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
template<size_t L>void input_text(std::array<char, L>& t) { input_text(t.data(), L - 1); }
bool horizontal_drag_bar(int& value, int min, int max, int page);
bool vertical_drag_bar(int& value, int min, int max, int page);
bool vertical_drag_button(int& pos, int row_height);

template<class T>
bool horizontal_drag_bar(T& value, int min, int max, int page) {
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
