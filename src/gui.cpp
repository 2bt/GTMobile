#include "gui.hpp"
#include "log.hpp"


namespace gui {
namespace {

DrawContext g_dc;
gfx::Image  g_img;

ivec2       g_cursor_min;
ivec2       g_cursor_max;
ivec2       g_item_size;
bool        g_same_line;


float       g_refresh_rate = 60.0f;
int         g_hold_count;
bool        g_hold;
ivec2       g_touch_pos;
ivec2       g_touch_prev_pos;
bool        g_touch_pressed;
bool        g_touch_prev_pressed;
void const* g_id;
void const* g_active_item;

enum class ButtonState { Normal, Hover, Active };


bool just_pressed() {
    return g_touch_pressed && !g_touch_prev_pressed;
}
bool just_released() {
    return !g_touch_pressed && g_touch_prev_pressed;
}
bool box_touched(Box const& box) {
    return (g_touch_pressed | g_touch_prev_pressed) && box.contains(g_touch_pos);
}


Box item_box() {
    ivec2 size = g_item_size;
    ivec2 pos;
    if (g_same_line) {
        g_same_line = false;
        pos = {g_cursor_max.x, g_cursor_min.y};
        if (g_cursor_max.y - g_cursor_min.y > size.y) {
            pos.y += (g_cursor_max.y - g_cursor_min.y - size.y) / 2;
        }
        g_cursor_max = max(g_cursor_max, pos + size);
    }
    else {
        pos = {g_cursor_min.x, g_cursor_max.y};
        g_cursor_min.y = g_cursor_max.y;
        g_cursor_max = pos + size;
    }
    return { pos, size };
}


bool button_helper(Box const& box, bool active) {
    g_hold = false;
    ButtonState state = active ? ButtonState::Active : ButtonState::Normal;
    bool clicked = false;
    if (g_active_item == nullptr && box_touched(box)) {
        state = ButtonState::Hover;
        if (box.contains(g_touch_prev_pos)) {
            if (++g_hold_count > g_refresh_rate * 0.25f) g_hold = true;
        }
        else g_hold_count = 0;
        if (just_released()) clicked = true;
    }
    g_dc.color(state == ButtonState::Active ? DARK_GREEN :
               state == ButtonState::Hover  ? GREEN :
                                              DARK_GREY);
    g_dc.box(box, 0);
    return clicked;
}


} // namespace



void init() {
    g_img.init("gui.png");
}

void free() {
    g_img.free();
}

void set_refresh_rate(float refresh_rate) {
    g_refresh_rate = refresh_rate;
}

DrawContext& get_draw_context() {
    return g_dc;
}

void touch(int x, int y, bool pressed) {
    g_touch_pos = {x, y};
    g_touch_pressed = pressed;
}

void begin_frame() {
//    ++g_input_cursor_blink;

    g_cursor_min = {};
    g_cursor_max = {};
    g_same_line = false;
    if (!(g_touch_pressed | g_touch_prev_pressed)) {
        g_active_item = nullptr;
        g_hold_count = 0;
    }
//    if (touch_just_pressed() && g_input_text_str) {
//        g_input_text_str = nullptr;
//        android::hide_keyboard();
//    }

}
void end_frame() {
    gfx::draw(g_dc, g_img);
    g_dc.clear();

    g_touch_prev_pos     = g_touch_pos;
    g_touch_prev_pressed = g_touch_pressed;
}

void same_line(bool same_line) {
    g_same_line = same_line;
}

void item_size(ivec2 size) {
    g_item_size = size;
}

bool button(Icon icon, bool active) {
    Box box = item_box();
    bool clicked = button_helper(box, active);
    int i = static_cast<int>(icon);
    g_dc.color(WHITE);
    g_dc.rect(box.pos + box.size / 2 - 8, 16, { i % 8 * 16, i / 8 * 16 });
    return clicked;
}




/////////////////
// DrawContext //
/////////////////
void DrawContext::fill(Box const& box) {
    i16vec2 uv(1, 65); // a white pixel
    auto i0 = add_vertex({ box.pos, uv, m_color });
    auto i1 = add_vertex({ box.pos + box.size.xo(), uv, m_color });
    auto i2 = add_vertex({ box.pos + box.size, uv, m_color });
    auto i3 = add_vertex({ box.pos + box.size.oy(), uv, m_color });
    m_indices.insert(m_indices.end(), { i0, i1, i2, i0, i2, i3 });
}
void DrawContext::box(Box const& box, int style) {
    if (style == 0) {
        fill({box.pos, box.size - 1});
        return;
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


} // namespace gui
