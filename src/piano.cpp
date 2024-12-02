#include "piano.hpp"
#include "app.hpp"
#include "song_view.hpp"


namespace piano {
namespace {

gt::Song& g_song = app::song();
int  g_instrument = 1;
int  g_scroll     = 14 * 3; // show octave 3 and 4
int  g_note       = 48;
bool g_note_on;
bool g_gate;

}


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
        int space = app::canvas_height() - 12 - app::BUTTON_HEIGHT * 2;
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
            if (gui::button(str, i == g_instrument)) {
                g_instrument = i;
                show_instrument_select = false;
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
