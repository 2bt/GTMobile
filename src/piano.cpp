#include "piano.hpp"
#include "app.hpp"
#include "song_view.hpp"
#include <cassert>


namespace piano {
namespace {

gt::Song& g_song       = app::song();
int       g_instrument = 1;
int       g_drag_instr = 0;
int       g_scroll     = 14 * 3; // show octave 3 and 4
int       g_note       = 48;
bool      g_note_on;
bool      g_gate;


void shuffle_instruments(size_t i, size_t j) {
    if (i == j) return;

    // init instrument mapping
    std::array<uint8_t, gt::MAX_INSTR> mapping;
    for (uint8_t i = 0; i < gt::MAX_INSTR; ++i) mapping[i] = i;
    if (i < j) {
        std::rotate(mapping.begin() + i, mapping.begin() + j, mapping.begin() + j + 1);
    } else if (i > j) {
        std::rotate(mapping.begin() + j, mapping.begin() + j + 1, mapping.begin() + i + 1);
    }

    // apply mapping
    g_instrument = mapping[g_instrument];
    auto copy = g_song.instruments;
    for (int i = 0; i < gt::MAX_INSTR; ++i) g_song.instruments[mapping[i]] = copy[i];
    for (gt::Pattern& patt : g_song.patterns) {
        for (gt::PatternRow& r : patt.rows) {
            r.instr = mapping[r.instr];
            if (r.command >= gt::CMD_SETWAVEPTR && r.command <= gt::CMD_SETFILTERPTR) {
                r.data = mapping[r.data];
            }
        }
    }
    for (int i = 0; i < gt::MAX_TABLELEN; ++i) {
        uint8_t lval = g_song.ltable[gt::WTBL][i];
        if ((lval & 0xf0) != 0xf0) continue;
        uint8_t cmd = lval & 0x0f;
        if (cmd >= gt::CMD_SETWAVEPTR && cmd <= gt::CMD_SETFILTERPTR) {
            uint8_t& rval = g_song.rtable[gt::WTBL][i];
            rval = mapping[rval];
        }
    }
}


ivec2 drag_button(char const* label) {
    gui::Box box = gui::item_box();
    gui::DrawContext& dc = gui::draw_context();
    dc.rgb(color::BUTTON_DRAGGED);
    dc.box(box, gui::BoxStyle::Normal);
    dc.rgb(color::WHITE);
    dc.text(gui::text_pos(box, label), label);

    ivec2 p = gui::touch::pos() - box.pos;
    return {
        div_floor(p.x, box.size.x),
        div_floor(p.y, box.size.y),
    };
}


} // namespace


int instrument() {
    return g_instrument;
}
int note() {
    return g_note;
}
bool note_on() {
    return g_note_on;
}


void draw() {

    static bool show_instrument_select = false;
    g_note_on = false;

    gui::DrawContext& dc = gui::draw_context();
    char str[32];

    gui::cursor({ 0, app::canvas_height() - HEIGHT });
    gui::item_size(app::CANVAS_WIDTH);
    gui::separator();

    // instrument button
    gui::item_size({ 8 * 18 + 12, app::BUTTON_HEIGHT });
    gui::align(gui::Align::Left);
    sprintf(str, "%02X %s", g_instrument, g_song.instruments[g_instrument].name.data());
    gui::button_style(gui::ButtonStyle::Normal);
    if (gui::button(str)) show_instrument_select = true;
    gui::align(gui::Align::Center);

    // piano scroll bar
    gui::same_line();
    gui::item_size({ app::CANVAS_WIDTH - gui::cursor().x, app::BUTTON_HEIGHT });
    gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
    gui::horizontal_drag_bar(g_scroll, 0, PIANO_STEP_COUNT - PIANO_PAGE, PIANO_PAGE);


    // instrument window
    if (show_instrument_select) {
        enum {
            COL_W = 12 + 8 * 18,
        };
        int space = app::canvas_height() - gui::FRAME_WIDTH * 4 - app::BUTTON_HEIGHT * 2;
        int row_h = std::min<int>(space / 32, app::BUTTON_HEIGHT);

        gui::begin_window({ COL_W * 2, row_h * 32 + app::BUTTON_HEIGHT * 2 + gui::FRAME_WIDTH * 2 });
        gui::item_size({ COL_W * 2, app::BUTTON_HEIGHT });
        gui::text("INSTRUMENT SELECT");
        gui::separator();

        gui::align(gui::Align::Left);
        gui::item_size({ COL_W, row_h });
        for (int n = 0; n < gt::MAX_INSTR; ++n) {
            gui::same_line(n % 2 == 1);
            if (n == 0) {
                gui::item_box();
                continue;
            }
            int i = n % 2 * 32 + n / 2;
            gt::Instrument const& instr = g_song.instruments[i];
            sprintf(str, "%02X %s", i, instr.name.data());
            bool set = instr.ptr[gt::WTBL] | instr.ptr[gt::PTBL] | instr.ptr[gt::FTBL];
            gui::button_style(set ? gui::ButtonStyle::Normal : gui::ButtonStyle::Shaded);

            if (i == g_drag_instr) {
                int prev_instr = g_drag_instr;
                ivec2 mov = drag_button(str);
                ivec2 pos = ivec2(n % 2, n / 2) + mov;
                pos = clamp(pos, { 0, 0 }, { 1, 31 });
                g_drag_instr = pos.x * 32 + pos.y;
                if (g_drag_instr == 0) g_drag_instr = prev_instr;
                shuffle_instruments(prev_instr, g_drag_instr);
                if (gui::touch::just_released()) g_drag_instr = 0;
            }
            else {
                if (gui::button(str, i == g_instrument)) {
                    g_instrument = i;
                    show_instrument_select = false;
                }
                else if (gui::hold()) {
                    g_drag_instr = i;
                    gui::set_active_item(&g_drag_instr);
                }
            }

        }
        gui::button_style(gui::ButtonStyle::Normal);
        gui::item_size({ COL_W * 2, app::BUTTON_HEIGHT });
        gui::separator();
        gui::align(gui::Align::Center);
        if (gui::button("CLOSE")) show_instrument_select = false;
        gui::end_window();
    }

    auto loop_keys = [&](auto f) {
        for (int i = -1; i < 30; i += 1) {
            constexpr int NOTE_LUT[14] = { 0, 1, 2, 3, 4, -1, 5, 6, 7, 8, 9, 10, 11, -1, };
            int n = i + g_scroll;
            int note = NOTE_LUT[n % 14];
            if (note == -1) continue;
            f(i, n, note + n / 14 * 12);
        }
    };

    // check touch
    bool prev_gate = g_gate;
    int  prev_note = g_note;

    g_gate = false;

    int y_pos = gui::cursor().y;

    if (!gui::has_active_item() && gui::touch::pressed()) {
        loop_keys([&](int i, int n, int note) {
            gui::Box b = {
                { i * KEY_HALF_WIDTH, y_pos },
                { KEY_HALF_WIDTH * 2, KEY_HALF_HEIGHT },
            };
            if (n % 2 == 0) b.pos.y += KEY_HALF_HEIGHT;
            if (gui::touch::touched(b)) {
                g_gate = true;
                g_note = note;
            }
        });
    }
    int chan = song_view::channel();
    if (g_gate && (!prev_gate || g_note != prev_note)) {
        app::player().play_test_note(g_note + gt::FIRSTNOTE, g_instrument, chan);
        g_note_on = true;
    }
    if (!g_gate && prev_gate) {
        app::player().release_note(chan);
    }

    // draw white keys
    loop_keys([&](int i, int n, int note) {
        if (n % 2 == 1) return;
        dc.rgb(0xbbbbbb);
        if (g_gate && g_note == note) {
            dc.rgb(color::BUTTON_ACTIVE);
        }
        gui::Box b = {
            { i * KEY_HALF_WIDTH, y_pos },
            { KEY_HALF_WIDTH * 2, KEY_HALF_HEIGHT * 2 - 1 },
        };
        dc.box(b, gui::BoxStyle::PianoKey);
        if (note % 12 == 0) {
            char str[2] = { char('0' + note / 12) };
            dc.rgb(color::DARK_GREY);
            dc.text({ b.pos.x + KEY_HALF_WIDTH - 4, y_pos + KEY_HALF_HEIGHT * 2 - 13 }, str);
        }
    });
    // draw black keys
    loop_keys([&](int i, int n, int note) {
        if (n < 0 || n % 2 == 0) return;
        dc.rgb(0x111111);
        if (g_gate && g_note == note) {
            dc.rgb(color::BUTTON_ACTIVE);
        }
        gui::Box b = {
            { i * KEY_HALF_WIDTH, y_pos },
            { KEY_HALF_WIDTH * 2, KEY_HALF_HEIGHT },
        };
        dc.box(b, gui::BoxStyle::PianoKey);
    });
}


} // namespace piano
