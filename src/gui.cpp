#include "gui.hpp"
#include "log.hpp"
#include "app.hpp"
#include "platform.hpp"
#include <cstdarg>
#include <cstring>
#include <cassert>
#include <array>


namespace gui {
namespace {

constexpr float HOLD_TIME = 0.333f;

struct Window {
    gfx::Mesh mesh;
    ivec2     cursor_min;
    ivec2     cursor_max;
};

std::vector<Window> g_windows = { {} };
size_t              g_window_index;
size_t              g_max_window_index;
size_t              g_last_max_window_count;


gfx::Image   g_img;
DrawContext  g_dc;

ivec2        g_cursor_min;
ivec2        g_cursor_max;
ivec2        g_item_size;
bool         g_same_line;
Align        g_align;
ColorTheme   g_color_theme = {
    color::BUTTON_NORMAL,
    color::BUTTON_ACTIVE,
    color::BUTTON_PRESSED,
    color::BUTTON_RELEASED,
};
ButtonStyle  g_button_style;
DragBarStyle g_drag_bar_style;


float        g_frame_time = 1.0f / 60.0f;
float        g_hold_time;
bool         g_hold;
ivec2        g_touch_pos;
ivec2        g_touch_prev_pos;
bool         g_touch_pressed;
bool         g_touch_prev_pressed;
void const*  g_id;
void const*  g_active_item;
char*        g_input_text_str = nullptr;
int          g_input_text_len;
int          g_input_text_pos;
float        g_input_cursor_blink;
bool         g_disabled;
ivec2        g_drag_start;



std::array<char, 256> g_text_buffer;
char const* print_to_text_buffer(const char* fmt, va_list args) {
    vsnprintf(g_text_buffer.data(), g_text_buffer.size(), fmt, args);
    return g_text_buffer.data();
}

void const* get_id(void const* addr) {
    if (g_id) {
        addr = g_id;
        g_id = nullptr;
   }
   return addr;
}

void button_color(ButtonState state, bool active) {
    g_dc.rgb(state == ButtonState::Normal && !active ? g_color_theme.button_normal :
             state == ButtonState::Normal && active  ? g_color_theme.button_active :
             state == ButtonState::Pressed           ? g_color_theme.button_pressed :
           /*state == ButtonState::Released*/          g_color_theme.button_released);
}

} // namespace


// low-level
DrawContext& draw_context() { return g_dc; }

Box item_box() {
    ivec2 size = g_item_size;
    ivec2 pos;
    if (g_same_line) {
        g_same_line = false;
        pos = {g_cursor_max.x, g_cursor_min.y};
        g_cursor_max = max(g_cursor_max, pos + size);
    }
    else {
        pos = {g_cursor_min.x, g_cursor_max.y};
        g_cursor_min.y = g_cursor_max.y;
        g_cursor_max = pos + size;
    }
    return { pos, size };
}

ivec2 text_pos(Box const& box, char const* text) {
    int y = (box.size.y - 7) / 2;
    switch (g_align) {
    case Align::Left:
        return box.pos + ivec2(6, y);
    case Align::Center:
        return box.pos + ivec2(box.size.x / 2 - g_dc.text_width(text) / 2, y);
    default: assert(0);
    }
}

ButtonState button_state(Box const& box, void const* addr) {
    g_hold = false;
    if (g_disabled) return ButtonState::Normal;
    void const* id = get_id(addr);
    if (id) {
        if (g_active_item == nullptr && touch::just_touched(box)) {
            g_active_item = id;
            g_drag_start = g_touch_pos;
            return ButtonState::Pressed;
        }
        if (g_active_item != id) return ButtonState::Normal;
        if (touch::just_released()) return ButtonState::Released;
        return ButtonState::Pressed;
    }

    if (g_active_item || !touch::touched(box)) return ButtonState::Normal;

    // hold
    if (box.contains(g_touch_prev_pos)) {
        if (g_hold_time >= 0) {
            g_hold_time += g_frame_time;
            if (g_hold_time > HOLD_TIME) {
                g_hold = true;
                g_hold_time = -1;
            }
        }
    }
    else g_hold_time = 0;

    if (touch::just_released()) return ButtonState::Released;
    return ButtonState::Pressed;
}



void touch_event(int x, int y, bool pressed) {
    g_touch_pos = {x, y};
    g_touch_pressed = pressed;
}


void key_event(int key, int unicode) {
    if (!g_input_text_str) return;
    switch (key) {
    case KEYCODE_DEL:
        g_input_cursor_blink = 0;
        if (g_input_text_pos > 0) g_input_text_str[--g_input_text_pos] = '\0';
        return;
    case KEYCODE_ENTER:
        platform::show_keyboard(false);
        g_input_text_str = nullptr;
        return;
    default: break;
    }

    if (unicode == 0) return;
    if (g_input_text_pos >= g_input_text_len) return;

    g_input_cursor_blink = 0;
    int c = unicode;
    if (c >= ' ' && c < '~' && c != '/') {
        g_input_text_str[g_input_text_pos++] = c;
    }
}



namespace touch {
    ivec2 pos() { return g_touch_pos; }
    bool pressed() {
        if (g_window_index != g_last_max_window_count) return false;
        return g_touch_pressed;
    }
    bool just_pressed() {
        if (g_window_index != g_last_max_window_count) return false;
        return g_touch_pressed && !g_touch_prev_pressed;
    }
    bool just_released() {
        if (g_window_index != g_last_max_window_count) return false;
        return !g_touch_pressed && g_touch_prev_pressed;
    }
    bool touched(Box const& box) {
        if (g_window_index != g_last_max_window_count) return false;
        return (g_touch_pressed | g_touch_prev_pressed) && box.contains(g_touch_pos);
    }
    bool just_touched(Box const& box) {
        if (g_window_index != g_last_max_window_count) return false;
        return just_pressed() && box.contains(g_touch_pos);
    }
} // namespace touch



void init() {
    g_img.init("gui.png");
    g_dc.font(0);
}

void free() {
    g_img.free();
}

void set_refresh_rate(float refresh_rate) {
    g_frame_time = 1.0f / refresh_rate;
}

float frame_time() { return g_frame_time; }

void begin_frame() {

    g_window_index = 0;
    g_max_window_index = 0;
    g_dc.mesh(g_windows[0].mesh);

    g_cursor_min = {};
    g_cursor_max = {};
    g_same_line = false;
    if (!g_touch_pressed && !g_touch_prev_pressed) {
        g_active_item = nullptr;
        g_hold_time = 0;
    }

    g_input_cursor_blink += g_frame_time * 2.0f;
    g_input_cursor_blink -= (int) g_input_cursor_blink;
    if (g_touch_pressed && !g_touch_prev_pressed && g_input_text_str) {
        g_input_text_str = nullptr;
        platform::show_keyboard(false);
    }
}

void end_frame() {
    assert(g_window_index == 0);
    int n = std::min(g_last_max_window_count, g_max_window_index);
    for (int i = 0; i <= n; ++i) {
        Window& w = g_windows[i];

        if (i == n - 1) {
            g_dc.mesh(w.mesh);
            g_dc.rgb(0);
            g_dc.alpha(180);
            g_dc.fill({ {}, { app::CANVAS_WIDTH, app::canvas_height() } });
            g_dc.alpha(255);
        }

        gfx::draw(w.mesh, g_img);
        w.mesh.vertices.clear();
        w.mesh.indices.clear();
    }
    g_last_max_window_count = g_max_window_index;

    g_touch_prev_pos     = g_touch_pos;
    g_touch_prev_pressed = g_touch_pressed;
}

void begin_window() {
    g_windows[g_window_index].cursor_min = g_cursor_min;
    g_windows[g_window_index].cursor_max = g_cursor_max;
    if (++g_window_index > g_max_window_index) {
        g_max_window_index = g_window_index;
        if (g_windows.size() < g_max_window_index + 1) {
            g_windows.emplace_back();
        }
    }
    g_dc.mesh(g_windows[g_window_index].mesh);
}
Box begin_window(ivec2 size) {
    begin_window();
    ivec2 pos = ivec2(app::CANVAS_WIDTH, app::canvas_height()) / 2 - size / 2;
    g_dc.rgb(color::FRAME);
    g_dc.box({ pos - 6, size + 12 }, BoxStyle::Window);
    cursor(pos);
    return { pos, size };
}
void end_window() {
    --g_window_index;
    g_dc.mesh(g_windows[g_window_index].mesh);
    g_cursor_min = g_windows[g_window_index].cursor_min;
    g_cursor_max = g_windows[g_window_index].cursor_max;
}


void cursor(ivec2 pos) {
    g_cursor_min = pos;
    g_cursor_max = pos;
}
ivec2 cursor() {
    if (g_same_line) return { g_cursor_max.x, g_cursor_min.y };
    return { g_cursor_min.x, g_cursor_max.y };
}
void item_size(ivec2 size) {
    g_item_size = size;
}
void same_line(bool same_line) {
    g_same_line = same_line;
}
void id(void const* addr) {
    if (!g_id) g_id = addr;
}
ColorTheme& color_theme() {
    return g_color_theme;
}
void button_style(ButtonStyle style) {
    g_button_style = style;
}
void drag_bar_style(DragBarStyle style) {
    g_drag_bar_style = style;
}
void set_active_item(void const* id) {
    g_active_item = id;
}
bool has_active_item() {
    return g_active_item != nullptr;
}
void align(Align a) {
    g_align = a;
}
bool hold() {
    return g_hold;
}
void disabled(bool disabled) {
    g_disabled = disabled;
    g_dc.alpha(disabled ? 100 : 255);
}
bool get_disabled() {
    return g_disabled;
}

inline BoxStyle box_style(ButtonStyle s) {
    switch (s) {
        case ButtonStyle::Shaded: return BoxStyle::Shaded;
        case ButtonStyle::Tab: return BoxStyle::Tab;
        case ButtonStyle::ShadedTab: return BoxStyle::ShadedTab;
        case ButtonStyle::RadioLeft: return BoxStyle::RadioLeft;
        case ButtonStyle::RadioCenter: return BoxStyle::RadioCenter;
        case ButtonStyle::RadioRight: return BoxStyle::RadioRight;
        default:
        case ButtonStyle::Normal: return BoxStyle::Normal;
    }
}

void separator(bool leave_gap) {
    ivec2 size = g_item_size;
    ivec2 pos;
    ivec2 padding = 1;
    if (g_same_line) {
        size.x = FRAME_WIDTH;
        if (!leave_gap) padding.y = -1;
        pos = {g_cursor_max.x, g_cursor_min.y};
        g_cursor_max = max(g_cursor_max, pos + size);
    }
    else {
        size.y = FRAME_WIDTH;
        if (!leave_gap) padding.x = -1;
        pos = {g_cursor_min.x, g_cursor_max.y};
        g_cursor_min.y = g_cursor_max.y;
        g_cursor_max = pos + size;
    }
    g_dc.rgb(color::FRAME);
    g_dc.fill({ pos + padding, size - padding * 2 });
}

bool button(Icon icon, bool active) {
    Box box = item_box();
    ButtonState state = button_state(box);
    button_color(state, active);
    g_dc.box(box, box_style(g_button_style));

    int i = int(icon);
    g_dc.rgb(color::WHITE);
    g_dc.rect(box.pos + box.size / 2 - 8, 16, { i % 16 * 16, i / 16 * 16 });
    return state == ButtonState::Released;
}
bool button(char const* label, bool active) {
    Box box = item_box();
    ButtonState state = button_state(box);
    if (g_button_style == ButtonStyle::TableCell || g_button_style == ButtonStyle::PaddedTableCell) {
        Box b = box;
        if (g_button_style == ButtonStyle::PaddedTableCell) {
            b.pos.y  += 1;
            b.size.y -= 2;
        }
        b.pos.x  += 1;
        b.size.x -= 2;
        g_dc.rgb(color::BACKGROUND_ROW);
        g_dc.fill(b);
        if (active || state != ButtonState::Normal) {
            button_color(state, active);
            g_dc.box(b, BoxStyle::Cursor);
        }
    }
    else {
        button_color(state, active);
        g_dc.box(box, box_style(g_button_style));
    }

    g_dc.rgb(color::WHITE);
    g_dc.text(text_pos(box, label), label);
    return state == ButtonState::Released;
}

void text(char const* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char const* str = print_to_text_buffer(fmt, args);
    va_end(args);
    Box box = item_box();
    g_dc.rgb(color::WHITE);
    g_dc.text(text_pos(box, str), str);
}
void input_text(char* str, int len) {
    Box box = item_box();
    ButtonState state = button_state(box);
    if (state == ButtonState::Released) {
        platform::show_keyboard(true);
        g_input_cursor_blink = 0;
        g_input_text_str = str;
        g_input_text_len = len;
        g_input_text_pos = strlen(g_input_text_str);
    }

    bool active = g_input_text_str == str;
    button_color(state, active);
    g_dc.box(box, BoxStyle::Text);
    ivec2 p = box.pos + ivec2(6, box.size.y / 2 - 4);
    g_dc.rgb(color::WHITE);
    g_dc.text(p, str);
    // cursor
    if (g_input_text_str == str && g_input_cursor_blink < 0.5f) {
        g_dc.text(p + ivec2(strlen(str) * 8, 0), "\x7f");
    }
}

bool horizontal_drag_bar(int& value, int min, int max, int page) {
    max = std::max(max, min);
    Box box = item_box();
    ButtonState state = button_state(box, &value);
    bool is_active = state != ButtonState::Normal;

    int range = max - min;
    int handle_w = box.size.x * page / (range + page);
    handle_w = std::max(handle_w, 16);
    int move_w = box.size.x - handle_w;

    int old_value = value;
    if (is_active && range > 0) {
        int dv = (g_touch_pos.x - g_drag_start.x) * range / move_w;
        if (range > move_w) g_drag_start.x = g_touch_pos.x;
        else g_drag_start.x += dv * move_w / range;
        value = clamp(value + dv, min, max);
    }
    int handle_x = range == 0 ? 0 : (value - min) * move_w / range;

    g_dc.rgb(color::DRAG_BG);
    g_dc.box(box, BoxStyle::Normal);
    if (move_w > 0) {
        if (g_drag_bar_style == DragBarStyle::Scrollbar) {
            g_dc.rgb(is_active ? color::DRAG_HANDLE_ACTIVE : color::DRAG_HANDLE_NORMAL);
            g_dc.box({ box.pos + ivec2(handle_x, 0), { handle_w, box.size.y } }, BoxStyle::Normal);
            g_dc.rgb(color::DRAG_ICON);
            int i = int(Icon::HGrab);
            g_dc.rect(box.pos + ivec2(handle_x, 0) + ivec2(handle_w, box.size.y) / 2 - 8, 16,
                    { i % 8 * 16, i / 8 * 16 });
        }
        else {
            g_dc.rgb(is_active ? g_color_theme.button_pressed : g_color_theme.button_normal);
            g_dc.box({ box.pos + ivec2(handle_x, 0), { handle_w, box.size.y } }, BoxStyle::Normal);
        }
    }
    return value != old_value;
}


bool vertical_drag_bar(int& value, int min, int max, int page) {
    max = std::max(max, min);
    Box box = item_box();
    ButtonState state = button_state(box, &value);
    bool is_active = state != ButtonState::Normal;

    int range = max - min;
    int handle_h = box.size.y * page / (range + page);
    handle_h = std::max(handle_h, 16);
    int move_h = box.size.y - handle_h;

    int old_value = value;
    if (is_active && range > 0) {
        int dv = (g_touch_pos.y - g_drag_start.y) * range / move_h;
        if (range > move_h) g_drag_start.y = g_touch_pos.y;
        else g_drag_start.y += dv * move_h / range;
        value = clamp(value + dv, min, max);
    }
    int handle_y = range == 0 ? 0 : (value - min) * move_h / range;

    g_dc.rgb(color::DRAG_BG);
    g_dc.box(box, BoxStyle::Normal);
    if (move_h > 0) {
        if (g_drag_bar_style == DragBarStyle::Scrollbar) {
            g_dc.rgb(is_active ? color::DRAG_HANDLE_ACTIVE : color::DRAG_HANDLE_NORMAL);
            g_dc.box({ box.pos + ivec2(0, handle_y), { box.size.x, handle_h } }, BoxStyle::Normal);
            g_dc.rgb(color::DRAG_ICON);
            int i = int(Icon::VGrab);
            g_dc.rect(box.pos + ivec2(0, handle_y) + ivec2(box.size.x, handle_h) / 2 - 8, 16,
                      { i % 16 * 16, i / 16 * 16 });
        }
        else {
            g_dc.rgb(is_active ? g_color_theme.button_pressed : g_color_theme.button_normal);
            g_dc.box({ box.pos + ivec2(0, handle_y), { box.size.x, handle_h } }, BoxStyle::Normal);
        }
    }
    return value != old_value;
}
bool vertical_drag_button(int& pos, int row_height) {
    Box box = item_box();
    ButtonState state = button_state(box, &pos);
    bool is_active = state != ButtonState::Normal;
    int old_pos = pos;
    if (is_active) {
        int dy = g_touch_pos.y - (box.pos.y + box.size.y / 2);
        pos += dy / row_height;
    }
    g_dc.rgb(is_active ? color::DRAG_HANDLE_ACTIVE : color::DRAG_HANDLE_NORMAL);
    g_dc.box(box, BoxStyle::Normal);
    g_dc.rgb(color::DRAG_ICON);
    int i = int(Icon::VGrab);
    g_dc.rect(box.pos + box.size / 2 - 8, 16, { i % 16 * 16, i / 16 * 16 });
    return pos != old_pos;
}

bool slider(int width, char const* fmt, int& value, int min, int max, void const* addr) {
    enum { BUTTON_HEIGHT = 30 };
    int v = value;
    int old_v = value;
    int text_width = 0;
    if (fmt != nullptr) {
        char str[64] = {};
        snprintf(str, sizeof(str), fmt, v);
        text_width = 12 + strlen(str) * 8;
        item_size({ text_width, BUTTON_HEIGHT });
        text(str);
        same_line();
    }
    drag_bar_style(DragBarStyle::Normal);
    item_size({ width - text_width - BUTTON_HEIGHT * 2, BUTTON_HEIGHT });
    id(addr ? addr : &value);
    horizontal_drag_bar(v, min, max, 0);
    same_line();
    button_style(ButtonStyle::Normal);
    item_size(BUTTON_HEIGHT);
    bool prev_disabled = g_disabled;
    disabled(prev_disabled || v == min);
    if (button(Icon::Decrease)) v = std::max(min, v - 1);
    same_line();
    disabled(prev_disabled || v == max);
    if (button(Icon::Increase)) v = std::min(max, v + 1);
    disabled(prev_disabled);
    value = v;
    return v != old_v;
}
bool choose(int width, char const* desc, int& value, std::initializer_list<char const*> labels) {
    int old_v = value;

    if (desc != nullptr) {
        int text_width = 12 + strlen(desc) * 8;
        width -= text_width;
        item_size({ text_width, app::BUTTON_HEIGHT });
        text(desc);
        same_line();
    }

    const int count      = labels.size();
    const int item_width = width / count;
    const int rest_width = width - item_width * count;
    int a = 0;
    int i = 0;

    button_style(ButtonStyle::RadioLeft);
    for (auto label : labels) {
        int w = item_width;
        a += rest_width;
        if (a >= count) {
            a -= count;
            ++w;
        }
        item_size({ w, app::BUTTON_HEIGHT });

        if (button(label, value == i)) {
            value = i;
        }
        button_style(i + 2 == int(labels.size()) ? ButtonStyle::RadioRight : ButtonStyle::RadioCenter);
        same_line();
        ++i;
    }
    same_line(false);
    button_style(ButtonStyle::Normal);
    return value != old_v;
}




/////////////////
// DrawContext //
/////////////////
void DrawContext::box(Box const& box, BoxStyle style) {
    if (box.size.x < 4 || box.size.y < 4) return;
    ivec2 c = {
        std::min(8, box.size.x / 2),
        std::min(8, box.size.y / 2),
    };
    i16vec2 p0 = box.pos;
    i16vec2 p1 = box.pos + c;
    i16vec2 p2 = box.pos + box.size - c;
    i16vec2 p3 = box.pos + box.size;
    i16vec2 t0(int(style) % 16 * 16, int(style) / 16 * 16);
    i16vec2 t1 = t0 + c;
    i16vec2 t2 = t0 + 16 - c;
    i16vec2 t3 = t0 + 16;

    auto i0  = add_vertex({ p0.x, p0.y }, { t0.x, t0.y });
    auto i1  = add_vertex({ p1.x, p0.y }, { t1.x, t0.y });
    auto i2  = add_vertex({ p2.x, p0.y }, { t2.x, t0.y });
    auto i3  = add_vertex({ p3.x, p0.y }, { t3.x, t0.y });
    auto i4  = add_vertex({ p0.x, p1.y }, { t0.x, t1.y });
    auto i5  = add_vertex({ p1.x, p1.y }, { t1.x, t1.y });
    auto i6  = add_vertex({ p2.x, p1.y }, { t2.x, t1.y });
    auto i7  = add_vertex({ p3.x, p1.y }, { t3.x, t1.y });
    auto i8  = add_vertex({ p0.x, p2.y }, { t0.x, t2.y });
    auto i9  = add_vertex({ p1.x, p2.y }, { t1.x, t2.y });
    auto i10 = add_vertex({ p2.x, p2.y }, { t2.x, t2.y });
    auto i11 = add_vertex({ p3.x, p2.y }, { t3.x, t2.y });
    auto i12 = add_vertex({ p0.x, p3.y }, { t0.x, t3.y });
    auto i13 = add_vertex({ p1.x, p3.y }, { t1.x, t3.y });
    auto i14 = add_vertex({ p2.x, p3.y }, { t2.x, t3.y });
    auto i15 = add_vertex({ p3.x, p3.y }, { t3.x, t3.y });
    m_mesh->indices.insert(m_mesh->indices.end(), {
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


} // namespace gui
