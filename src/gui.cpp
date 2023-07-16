#include "gui.hpp"
#include "log.hpp"


namespace gui {
namespace {

DrawContext g_dc;
gfx::Image  g_img;

int         g_hold_time = 60.0f / 4;
int         g_hold_count;
bool        g_hold;
ivec2       g_touch_pos;
ivec2       g_touch_prev_pos;
bool        g_touch_pressed;
bool        g_touch_prev_pressed;
void const* g_id;
void const* g_active_item;

enum class ButtonSate { Normal, Hover, Active };


bool just_pressed() {
    return g_touch_pressed && !g_touch_prev_pressed;
}
bool just_released() {
    return !g_touch_pressed && g_touch_prev_pressed;
}
bool box_touched(Box const& box) {
    return (g_touch_pressed | g_touch_prev_pressed) && box.contains(g_touch_pos);
}


bool button_helper(Box const& box, bool active) {
    g_hold = false;
    ButtonSate s = active ? ButtonSate::Active : ButtonSate::Normal;
    bool clicked = false;
    if (g_active_item == nullptr && box_touched(box)) {
        s = ButtonSate::Hover;
        if (box.contains(g_touch_prev_pos)) {
            if (++g_hold_count > g_hold_time) g_hold = true;
        }
        else g_hold_count = 0;
        if (just_released()) clicked = true;
    }
    (void) s;
    g_dc.box(box, 1);
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
    g_hold_time = refresh_rate / 4;
}

DrawContext& get_draw_context() {
    return g_dc;
}

void touch(int x, int y, bool pressed) {
    g_touch_pos = {x, y};
    g_touch_pressed = pressed;
}

void end_frame() {
    gfx::draw(g_dc, g_img);
    g_dc.clear();

    g_touch_prev_pos     = g_touch_pos;
    g_touch_prev_pressed = g_touch_pressed;
}


bool button(Icon icon, bool active) {
//    Vec s = Vec(16);
//    Box box = item_box(s);
//    bool clicked = button_helper(box, active);
//    m_dc.rect(print_pos(box, s), s, { icon % 8 * 16, icon / 8 * 16 });
//    return clicked;
}




/////////////////
// DrawContext //
/////////////////
void DrawContext::box(Box const& box, int style) {
    if (style == 0) {
        i16vec2 uv(1, 65); // a white pixel
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


} // namespace gui
