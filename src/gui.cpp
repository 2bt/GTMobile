#include "gui.hpp"
#include "log.hpp"


namespace gui {
namespace {

DrawContext g_dc;

int g_hold_time = 60.0f / 4;


bool button_helper(Box const& box, bool active) {
//    m_hold = false;
//    Color c = m_button_theme->color_normal;
//    bool clicked = false;
//    if (m_active_item == nullptr && m_touch.box_touched(box)) {
//        c = m_button_theme->color_hover;
//        if (box.contains(m_touch.prev_pos)) {
//            if (++m_hold_count > g_hold_time) m_hold = true;
//        }
//        else m_hold_count = 0;
//        if (m_touch.just_released()) clicked = true;
//    }
//    else {
//        if (active) c = m_button_theme->color_active;
//    }
//    m_dc.rect(box.pos, box.size, c, m_button_theme->button_style);
//    return clicked;
}

} // namespace


void set_refresh_rate(float refresh_rate) {
    g_hold_time = refresh_rate / 4;
}

void touch(int x, int y, bool pressed) {
}


bool button(Icon icon, bool active) {
//    Vec s = Vec(16);
//    Box box = item_box(s);
//    bool clicked = button_helper(box, active);
//    m_dc.rect(print_pos(box, s), s, { icon % 8 * 16, icon / 8 * 16 });
//    return clicked;
}




} // namespace gui
