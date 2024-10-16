#include "piano.hpp"
#include "app.hpp"
#include "gui.hpp"
#include "song_view.hpp"


namespace piano {
namespace {

gt::Song& g_song = app::song();
int  g_instrument = 1;
int  g_scroll     = 14 * 3; // show octave 3 and 4
bool g_gate;
int  g_note;

}


int instrument() {
    return g_instrument;
}
int note() {
    return g_note;
}

bool draw(bool* follow) {
    static bool show_instrument_select = false;
    bool note_on = false;

    int piano_y = app::canvas_height() - HEIGHT;
    gui::DrawContext& dc = gui::draw_context();
    char str[32];

    gui::cursor({ 0, piano_y });
    piano_y += app::BUTTON_HEIGHT;

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
        int space = app::canvas_height() - 12 - app::BUTTON_HEIGHT * 2;
        int row_h = std::min<int>(space / 32, app::MAX_ROW_HEIGHT);
        enum {
            COL_W = 12 + 8 * 18,
        };
        gui::begin_window({ COL_W * 2, row_h * 32 + app::BUTTON_HEIGHT * 2 });
        gui::item_size({ COL_W * 2, app::BUTTON_HEIGHT });
        gui::text("SELECT INSTRUMENT");

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
            bool set = instr.ptr[0] | instr.ptr[1] | instr.ptr[2] | instr.ptr[3];
            gui::button_style(set ? gui::ButtonStyle::Normal : gui::ButtonStyle::Shaded);
            if (gui::button(str, i == g_instrument)) {
                g_instrument = i;
                show_instrument_select = false;
            }
        }
        gui::button_style(gui::ButtonStyle::Normal);
        gui::item_size({ COL_W * 2, app::BUTTON_HEIGHT });
        gui::align(gui::Align::Center);
        if (gui::button("CANCEL")) show_instrument_select = false;
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
        if (n % 2 == 1) return;
        dc.rgb(0xbbbbbb);
        if (g_gate && g_note == note) {
            dc.rgb(color::BUTTON_ACTIVE);
        }
        gui::Box b = {
            { i * KEY_HALF_WIDTH, piano_y },
            { KEY_HALF_WIDTH * 2, KEY_HALF_HEIGHT * 2 - 1 },
        };
        dc.box(b, gui::BoxStyle::PianoKey);
        if (note % 12 == 0) {
            char str[2] = { char('0' + note / 12) };
            dc.rgb(color::DARK_GREY);
            dc.text({ b.pos.x + KEY_HALF_WIDTH - 4, piano_y + KEY_HALF_HEIGHT * 2 - 13 }, str);
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
            { i * KEY_HALF_WIDTH, piano_y },
            { KEY_HALF_WIDTH * 2, KEY_HALF_HEIGHT },
        };
        dc.box(b, gui::BoxStyle::PianoKey);
    });

    // player buttons
    gt::Player& player = app::player();
    piano_y += KEY_HALF_HEIGHT * 2;
    gui::cursor({ 0, piano_y });
    gui::item_size(app::TAB_HEIGHT);


    static float backward_time = 0.0f;
    backward_time += gui::get_frame_time();
    if (gui::button(gui::Icon::FastBackward)) {
        if (backward_time < 0.5f) {
            for (int& x : player.m_start_song_pos) {
                if (x > 0) --x;
            }
        }
        player.m_start_patt_pos = {};
        if (player.is_playing()) player.play_song();
        else {
            player.m_current_song_pos = player.m_start_song_pos;
            player.m_current_patt_pos = {};
        }
        backward_time = 0.0f;
    }
    gui::same_line();

    if (gui::button(gui::Icon::Stop)) {
        player.m_start_song_pos.fill(song_view::song_position());
        player.m_start_patt_pos = {};
        player.stop_song();
    }

    // play button
    int w = app::CANVAS_WIDTH - app::TAB_HEIGHT * 4;
    if (follow) w -= app::TAB_HEIGHT;
    gui::item_size({ w, app::TAB_HEIGHT });
    bool is_playing = player.is_playing();
    gui::same_line();
    if (gui::button(gui::Icon::Play, is_playing)) {
        if (is_playing) {
            player.pause_song();
        }
        else {
            player.play_song();
        }
    }

    gui::item_size(app::TAB_HEIGHT);
    if (follow) {
        gui::same_line();
        if (gui::button(gui::Icon::Follow, *follow)) {
            *follow ^= 1;
        }
    }
    gui::same_line();

    bool loop = player.get_loop();
    if (gui::button(gui::Icon::Loop, loop)) {
        player.set_loop(!loop);
    }

    gui::same_line();
    if (gui::button(gui::Icon::FastForward)) {
        for (int& x : player.m_start_song_pos) {
            if (x < g_song.song_len - 1) ++x;
            else x = 0;
        }
        player.m_start_patt_pos = {};
        if (player.is_playing()) player.play_song();
        else {
            player.m_current_song_pos = player.m_start_song_pos;
            player.m_current_patt_pos = {};
        }
    }


    return note_on;
}


} // namespace piano
