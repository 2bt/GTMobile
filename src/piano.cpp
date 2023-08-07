#include "piano.hpp"
#include "app.hpp"
#include "gui.hpp"
#include "song_view.hpp"


namespace piano {
namespace {

int  g_instrument = 1;
int  g_scroll     = 14 * 3; // show octave 3 and 4
bool g_gate       = false;
int  g_note;

}


int instrument() {
    return g_instrument;
}
int note() {
    return g_note;
}


bool draw() {
    bool note_on = false;
    enum {
        KEY_HALF_WIDTH   = 12,
        KEY_HALF_HEIGHT  = 24,
        PIANO_PAGE       = app::CANVAS_WIDTH / KEY_HALF_WIDTH,
        PIANO_STEP_COUNT = 14 * 8 - 4,
    };

    int const piano_y = app::canvas_height() - KEY_HALF_HEIGHT * 2;
    gt::Song& song = app::song();
    gui::DrawContext& dc = gui::draw_context();
    char str[32];

    gui::cursor({ 0, piano_y - app::SCROLLBAR_WIDTH });

    // instrument button
    static bool show_instr_win = false;
    gui::item_size({ 8 * 18 + 12, app::SCROLLBAR_WIDTH });
    gui::align(gui::Align::Left);
    sprintf(str, "%02X %s", g_instrument, song.instr[g_instrument].name.data());
    if (gui::button(str, show_instr_win)) show_instr_win = true;

    // piano scroll bar
    gui::same_line();
    gui::item_size({ app::CANVAS_WIDTH - gui::cursor().x, app::SCROLLBAR_WIDTH });
    gui::drag_bar_theme(gui::DragBarTheme::Scrollbar);
    gui::horizontal_drag_bar(g_scroll, 0, PIANO_STEP_COUNT - PIANO_PAGE, PIANO_PAGE);


    // instrument window
    if (show_instr_win) {
        gui::Box box = gui::begin_window({ 8 * 19 * 2, 16 * 32 + 24 + 24 });

        gui::align(gui::Align::Center);
        gui::item_size({ box.size.x, 24 });
        gui::text("SELECT INSTRUMENT");

        gui::align(gui::Align::Left);
        gui::item_size({8 * 19, 16});
        for (int n = 0; n < gt::MAX_INSTR; ++n) {
            gui::same_line(n % 2 == 1);

            int i = n % 2 * 32 + n / 2;
            sprintf(str, "%02X %s", i, song.instr[i].name.data());
            if (gui::button(str, i == g_instrument)) {
                g_instrument = i;
                show_instr_win = false;
            }
        }
        gui::item_size({ box.size.x, 24 });
        gui::align(gui::Align::Center);
        if (gui::button("CANCEL")) show_instr_win = false;
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

    if (!gui::has_active_item() && gui::touch::pressed()) {
        loop_keys([&](int i, int n, int note) {
            gui::Box b = {
                { i * KEY_HALF_WIDTH, piano_y },
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
        note_on = true;
    }
    if (!g_gate && prev_gate) {
        app::player().release_note(chan);
    }

    // draw white keys
    loop_keys([&](int i, int n, int note) {
        if (n % 2 == 0) {
            dc.color(color::rgb(0xbbbbbb));
            if (g_gate && g_note == note) {
                dc.color(color::BUTTON_ACTIVE);
            }
            gui::Box b = {
                { i * KEY_HALF_WIDTH, piano_y },
                { KEY_HALF_WIDTH * 2, KEY_HALF_HEIGHT * 2 },
            };
            dc.box(b, gui::BoxStyle::PianoKey);
            if (note % 12 == 0) {
                char str[] = { char('0' + note / 12), '\0' };
                dc.color(color::DARK_GREY);
                dc.text({b.pos.x + KEY_HALF_WIDTH - 4, piano_y + 33}, str);
            }
        }
    });
    // draw black keys
    loop_keys([&](int i, int n, int note) {
        if (n % 2 == 1) {
            dc.color(color::rgb(0x333333));
            if (g_gate && g_note == note) {
                dc.color(color::BUTTON_ACTIVE);
            }
            gui::Box b = {
                { i * KEY_HALF_WIDTH, piano_y },
                { KEY_HALF_WIDTH * 2, KEY_HALF_HEIGHT },
            };
            dc.box(b, gui::BoxStyle::PianoKey);
        }
    });
    return note_on;
}


} // namespace piano