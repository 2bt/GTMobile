#pragma once
#include "gfx.hpp"
#include <initializer_list>


namespace color {

    constexpr uint32_t mix(uint32_t c1, uint32_t c2, float x) {
        float y = 1.0f - x;
        return uint8_t(uint8_t(c1 >> 16) * y + uint8_t(c2 >> 16) * x) << 16 |
               uint8_t(uint8_t(c1 >> 8) * y + uint8_t(c2 >> 8) * x) << 8 |
               uint8_t(uint8_t(c1) * y + uint8_t(c2) * x);
    }

    constexpr uint32_t BLACK       = 0x000000;
    constexpr uint32_t DARK_BLUE   = 0x1d2b53;
    constexpr uint32_t DARK_PURPLE = 0x7e2553;
    constexpr uint32_t DARK_GREEN  = 0x008751;
    constexpr uint32_t BROWN       = 0xab5236;
    constexpr uint32_t DARK_GREY   = 0x5f574f;
    constexpr uint32_t LIGHT_GREY  = 0xc2c3c7;
    constexpr uint32_t WHITE       = 0xffffff;
    constexpr uint32_t RED         = 0xff004d;
    constexpr uint32_t ORANGE      = 0xffa300;
    constexpr uint32_t YELLOW      = 0xffec27;
    constexpr uint32_t GREEN       = 0x00e436;
    constexpr uint32_t BLUE        = 0x29adff;
    constexpr uint32_t LAVENDER    = 0x83769c;
    constexpr uint32_t PINK        = 0xff77a8;
    constexpr uint32_t LIGHT_PEACH = 0xffccaa;

    constexpr uint32_t C64[] = {
        0x000000,
        0x626262,
        0x898989,
        0xadadad,
        0xffffff,
        0x9f4e44,
        0xcb7e75,
        0x6d5412,
        0xa1683c,
        0xc9d487,
        0x9ae29b,
        0x5cab5e,
        0x6abfc6,
        0x887ecb,
        0x50459b,
        0xa057a3,
    };

    constexpr uint32_t DRAG_BG            = 0x222222;
    constexpr uint32_t DRAG_HANDLE_NORMAL = 0x444444;
    constexpr uint32_t DRAG_HANDLE_ACTIVE = 0x666666;
    constexpr uint32_t DRAG_ICON          = 0x777777;

    constexpr uint32_t FRAME              = mix(DARK_GREY, BLACK, 0.3f);

    constexpr uint32_t BUTTON_NORMAL      = DARK_GREY;
    constexpr uint32_t BUTTON_ACTIVE      = C64[5];
    constexpr uint32_t BUTTON_PRESSED     = C64[6];
    constexpr uint32_t BUTTON_RELEASED    = C64[9];

    constexpr uint32_t BUTTON_ALT_ACTIVE  = C64[11];
    constexpr uint32_t BUTTON_ALT_PRESSED = C64[10];
    constexpr uint32_t BUTTON_DRAGGED     = C64[11];


    constexpr uint32_t CMD_NOTHING = mix(0x000000, LIGHT_GREY, 0.5f);
    constexpr uint32_t CMD_PORTA   = mix(PINK,     LIGHT_GREY, 0.5f);
    constexpr uint32_t CMD_VIBRATO = mix(0xff2030, LIGHT_GREY, 0.5f);
    constexpr uint32_t CMD_VOLUME  = mix(GREEN,    LIGHT_GREY, 0.5f);
    constexpr uint32_t CMD_WAVE    = mix(ORANGE,   LIGHT_GREY, 0.5f);
    constexpr uint32_t CMD_FILTER  = mix(BLUE,     LIGHT_GREY, 0.5f);
    constexpr uint32_t CMD_TEMPO   = mix(YELLOW,   LIGHT_GREY, 0.5f);

    constexpr uint32_t CMDS[16] = {
        CMD_NOTHING,
        CMD_PORTA,      // 1 portamento up
        CMD_PORTA,      // 2 portamento down
        CMD_PORTA,      // 3 tone portamento
        CMD_VIBRATO,    // 4 vibrato
        CMD_VOLUME,     // 5 attack/decay
        CMD_VOLUME,     // 6 sustain/release
        CMD_WAVE,       // 7 waveform reg
        CMD_WAVE,       // 8 wavetable ptr
        CMD_WAVE,       // 9 pulsetable ptr
        CMD_FILTER,     // A filtertable ptr
        CMD_FILTER,     // B filter control
        CMD_FILTER,     // C filter cutoff
        CMD_VOLUME,     // D master volume
        CMD_TEMPO,      // E funk tempo
        CMD_TEMPO,      // F tempo
    };

    constexpr uint32_t ROW_NUMBER     = 0xaaaaaa;
    constexpr uint32_t INSTRUMENT     = 0xaabbdd;
    constexpr uint32_t PLAYER_ROW     = mix(C64[12], BLACK, 0.7f);
    constexpr uint32_t BACKGROUND_ROW = 0x171717;
    constexpr uint32_t HIGHLIGHT_ROW  = 0x2b2b2b;
    constexpr uint32_t MARKED_ROW     = mix(BACKGROUND_ROW, BUTTON_PRESSED, 0.2f);

    constexpr uint32_t PALETTE[] = {
        WHITE,
        mix(0xff2030, LIGHT_GREY, 0.5f), // red
        mix(ORANGE,   LIGHT_GREY, 0.5f),
        mix(YELLOW,   LIGHT_GREY, 0.5f),
        mix(GREEN,    LIGHT_GREY, 0.5f),
        mix(BLUE,     LIGHT_GREY, 0.5f),
        mix(PINK,     LIGHT_GREY, 0.5f),
        mix(mix(0xff2030, LIGHT_GREY, 0.5f), BACKGROUND_ROW, 0.8f), // dark red
    };

} // namespace color


namespace gui {


struct ColorTheme {
    uint32_t button_normal;
    uint32_t button_active;
    uint32_t button_pressed;
    uint32_t button_released;
};


struct Box {
    bool contains(ivec2 p) const {
        return p.x >= pos.x && p.y >= pos.y &&
               p.x < pos.x + size.x && p.y < pos.y + size.y;
    }
    ivec2 pos;
    ivec2 size;
};


enum class BoxStyle {
    Fill = 128,
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
        i16vec2 uv(1, 1); // a white pixel
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
        if (g >= 128) return;
        int o = g < 32 ? 0 : m_font_offset;
        ivec2 uv(g % 32 * m_char_size.x, g / 32 * m_char_size.x + o);
        rect(pos, m_char_size, uv);
    }

    int text_width(char const* text) const {
        int w = 0;
        while (uint8_t c = *text++) {
            if (c == 0x80) { // half space
                w += m_char_size.x / 2;
                continue;
            }
            if (c == 0x81 || c == 0x82) { // set cmd color
                text++;
                continue;
            }
            w += m_char_size.x;
        }
        return w;
    }

    void text(ivec2 pos, char const* text) {
        ivec2 p = pos;
        while (uint8_t c = *text++) {
            if (c == 0x80) { // half space
                p.x += m_char_size.x / 2;
                continue;
            }
            if (c == 0x81) { // set cmd color
                rgb(color::CMDS[uint8_t(*text++)]);
                continue;
            }
            if ((c & 0xf0) == 0xf0) { // palette color
                rgb(color::PALETTE[c & 0xf]);
                continue;
            }
            character(p, c);
            p.x += m_char_size.x;
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
        m_font_offset = 32 * i;
    }

private:
    uint16_t add_vertex(i16vec2 pos, i16vec2 uv) {
        uint16_t index = m_mesh->vertices.size();
        m_mesh->vertices.push_back({ pos, uv, m_color });
        return index;
    }

    u8vec4     m_color       = { 255 };
    int        m_font_offset = 0;
    ivec2      m_char_size   = { 8, 8 };
    gfx::Mesh* m_mesh;

};


enum class Icon {
    Decrease = 144,
    Increase,
    MoveUp,
    MoveDown,
    VGrab,
    HGrab,

    AddRowAbove = 160,
    AddRowBelow,
    DeleteRow,
    JumpBack,
    Settings,
    X = 176,
    ChangeLength,
    Edit,
    Piano,

    Loop = 192,
    Stop,
    PlayPause,
    Play,
    FastBackward,
    FastForward,
    Follow,
    Record,

    Copy = 208,
    Paste,
    Share,

    Noise = 224,
    Pulse,
    Saw,
    Triangle,
    Test,
    Ring,
    Sync,
    Gate,
    Lowpass,
    Bandpass,
    Highpass,
};

enum class Align { Left, Center };

enum class ButtonStyle {
    Normal,
    Shaded,
    Tab,
    ShadedTab,
    PaddedTableCell,
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
    KEYCODE_ENTER       = 66,
    KEYCODE_DEL         = 67,
    KEYCODE_PRINTSCREEN = -1000,
};
enum {
    FRAME_WIDTH = 5,
};


void init();
void free();
void touch_event(int x, int y, bool pressed);
void key_event(int key, int unicode);
void set_refresh_rate(float refresh_rate);
float frame_time();


namespace touch {
    ivec2 pos();
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
void set_active_item(void const* id);
bool has_active_item();
void align(Align a);
void button_style(ButtonStyle style);
void drag_bar_style(DragBarStyle style);
ColorTheme& color_theme();

void separator(bool leave_gap = false);
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

bool slider(int width, char const* fmt, int& value, int min, int max, void const* addr=nullptr);
template<class T>
bool slider(int width, char const* fmt, T& value, int min, int max, void const* addr=nullptr) {
    int v = value;
    bool res = slider(width, fmt, v, min, max, &value);
    value = v;
    return res;
}

bool choose(int width, char const* desc, int& value, std::initializer_list<char const*> labels);
template<class T>
bool choose(int width, char const* desc, T& value, std::initializer_list<char const*> labels = { "OFF", "ON" }) {
    int v = int(value);
    bool res = choose(width, desc, v, labels);
    value = T(v);
    return res;
}


// low level functions
enum class ButtonState { Normal, Pressed, Released };
DrawContext& draw_context();
Box          item_box();
ButtonState  button_state(Box const& box, void const* addr = nullptr);
ivec2        text_pos(Box const& box, char const* text);

} // namespace
