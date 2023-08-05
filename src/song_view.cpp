#include "song_view.hpp"
#include "gtplayer.hpp"
#include "log.hpp"
#include "gui.hpp"
#include "app.hpp"
#include "sid.hpp"
#include <array>
#include <cstring>


namespace color {

    constexpr u8vec4 CMDS[16] = {
        mix(color::rgb(0x000000), color::WHITE, 0.2f),

        mix(color::rgb(0xff77a8), color::WHITE, 0.2f), // 1 portamento up
        mix(color::rgb(0xff77a8), color::WHITE, 0.2f), // 2 portamento down
        mix(color::rgb(0xff77a8), color::WHITE, 0.2f), // 3 tone portamento

        mix(color::rgb(0xff2030), color::WHITE, 0.2f), // 4 vibrato

        mix(color::rgb(0x00e436), color::WHITE, 0.2f), // 5 attack/decay
        mix(color::rgb(0x00e436), color::WHITE, 0.2f), // 6 sustain/release

        mix(color::rgb(0xffa300), color::WHITE, 0.2f), // 7 waveform reg
        mix(color::rgb(0xffa300), color::WHITE, 0.2f), // 8 wavetable ptr
        mix(color::rgb(0xffa300), color::WHITE, 0.2f), // 9 pulsetable ptr

        mix(color::rgb(0x29adff), color::WHITE, 0.2f), // A filtertable ptr
        mix(color::rgb(0x29adff), color::WHITE, 0.2f), // B filter control
        mix(color::rgb(0x29adff), color::WHITE, 0.2f), // C filter cutoff

        mix(color::rgb(0x00e436), color::WHITE, 0.2f), // D master volume

        mix(color::rgb(0xffec27), color::WHITE, 0.2f), // E funk tempo
        mix(color::rgb(0xffec27), color::WHITE, 0.2f), // F tempo
    };

    constexpr u8vec4 ROW_NUMBER     = color::rgb(0xaaaaaa);
    constexpr u8vec4 INSTRUMENT     = color::rgb(0xaabbdd);
    constexpr u8vec4 HIGHLIGHT_ROW  = color::rgb(0x1c1c1c);
    constexpr u8vec4 BACKGROUND_ROW = color::rgb(0x0c0c0c);
    constexpr u8vec4 PLAYER_ROW     = color::rgb(0x553311);


} // namespace color


namespace song_view {
namespace {


enum {
    SONG_NR = 0,
};

// settings
int                g_row_height         = 13; // 9-16, default: 13
int                g_row_highlight_step = 8;

std::array<int, 3> g_pattern_nums       = { 0, 1, 2 };

bool               g_follow             = true;
int                g_song_page_length   = 8;
int                g_song_scroll        = 0;
int                g_pattern_scroll     = 0;

int                g_instrument         = 1;
bool               g_recording          = false;
int                g_piano_scroll       = 14 * 3; // show octave 3 and 4
bool               g_piano_gate         = false;
int                g_piano_note;

int                g_cursor_chan        = 0;
int                g_cursor_pattern_row = -1;
int                g_cursor_song_row    = -1;


enum class Dialog {
    None,
    OrderEdit,
};
Dialog g_dialog = Dialog::None;



} // namespace


bool draw_piano() {
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

    gui::cursor({ 0, piano_y - 16 });

    // instrument button
    static bool show_instr_win = false;
    gui::item_size({ 8 * 19, 16 });
    gui::align(gui::Align::Left);
    sprintf(str, "%02X %s", g_instrument, song.instr[g_instrument].name.data());
    if (gui::button(str, show_instr_win)) show_instr_win = true;

    // piano scroll bar
    gui::same_line();
    gui::item_size({ app::CANVAS_WIDTH - gui::cursor().x, 16 });
    gui::horizontal_drag_bar(g_piano_scroll, 0, PIANO_STEP_COUNT - PIANO_PAGE, PIANO_PAGE);


    // instrument window
    if (show_instr_win) {
        gui::begin_window();

        ivec2 size = { 8 * 19 * 2, 16 * 33 };
        ivec2 pos  = ivec2(app::CANVAS_WIDTH, app::canvas_height()) / 2 - size / 2;
        dc.color(color::BLACK);
        dc.fill({ pos, size });
        gui::cursor(pos);


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
        gui::item_size({ 8 * 19 * 2, 16 });
        gui::align(gui::Align::Center);
        if (gui::button("CANCEL")) show_instr_win = false;
        gui::end_window();
    }


    auto loop_keys = [&](auto f) {
        for (int i = -1; i < 30; i += 1) {
            constexpr int NOTE_LUT[14] = { 0, 1, 2, 3, 4, -1, 5, 6, 7, 8, 9, 10, 11, -1, };
            int n = i + g_piano_scroll;
            int note = NOTE_LUT[n % 14];
            if (note == -1) continue;
            f(i, n, note + n / 14 * 12);
        }
    };

    // check touch
    bool prev_gate = g_piano_gate;
    int  prev_note = g_piano_note;

    g_piano_gate = false;

    if (!gui::has_active_item() && gui::touch::pressed()) {
        loop_keys([&](int i, int n, int note) {
            gui::Box b = {
                { i * KEY_HALF_WIDTH, piano_y },
                { KEY_HALF_WIDTH * 2, KEY_HALF_HEIGHT },
            };
            if (n % 2 == 0) b.pos.y += KEY_HALF_HEIGHT;
            if (gui::touch::touched(b)) {
                g_piano_gate = true;
                g_piano_note = note;
            }
        });
    }
    if (g_piano_gate && (!prev_gate || g_piano_note != prev_note)) {
        app::player().play_test_note(g_piano_note + gt::FIRSTNOTE, g_instrument, g_cursor_chan);
        note_on = true;
    }
    if (!g_piano_gate && prev_gate) {
        app::player().release_note(g_cursor_chan);
    }

    // draw white keys
    loop_keys([&](int i, int n, int note) {
        if (n % 2 == 0) {
            dc.color(color::rgb(0xbbbbbb));
            if (g_piano_gate && g_piano_note == note) {
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
            if (g_piano_gate && g_piano_note == note) {
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



void draw() {


    // buttons
    gui::cursor({ 268 + 16, 24 });
    gui::item_size({ 76, 24 });

    if (gui::button(gui::Icon::Follow, g_follow)) {
        g_follow = !g_follow;
    }

    bool is_playing = app::player().is_playing();
    if (gui::button(gui::Icon::Play, is_playing)) {
        if (is_playing) {
            app::player().stop_song();
        }
        else {
            app::player().init_song(0, gt::Player::PLAY_BEGINNING);
        }
    }



    // get player position info
    std::array<int, 3> player_song_rows;
    std::array<int, 3> player_pattern_nums;
    std::array<int, 3> player_pattern_rows;
    for (int c = 0; c < 3; ++c) {
        uint8_t  songptr;
        uint8_t  pattnum;
        uint32_t pattptr;
        app::player().get_chan_info(c, songptr, pattnum, pattptr);
        player_song_rows[c]    = std::max<int>(0, songptr - 1);
        player_pattern_nums[c] = pattnum;
        player_pattern_rows[c] = pattptr / 4;
    }

    gt::Song& song = app::song();
    int max_song_len =                    song.songlen[SONG_NR][0];
    max_song_len = std::max(max_song_len, song.songlen[SONG_NR][1]);
    max_song_len = std::max(max_song_len, song.songlen[SONG_NR][2]);
    max_song_len += 1; // include LOOP command

    int max_pattern_len =                       song.pattlen[g_pattern_nums[0]];
    max_pattern_len = std::max(max_pattern_len, song.pattlen[g_pattern_nums[1]]);
    max_pattern_len = std::max(max_pattern_len, song.pattlen[g_pattern_nums[2]]);


    gui::cursor({ 0, 24 });

    int height_left = app::canvas_height() - (gui::cursor().y + 48 + 16 + 16 + 24);
    int total_rows = height_left / g_row_height;
    g_song_page_length = clamp(g_song_page_length, 0, total_rows);
    int pattern_page_length = total_rows - g_song_page_length;



    if (is_playing && g_follow) {
        // auto scroll
        g_pattern_nums       = player_pattern_nums;
        g_song_scroll        = player_song_rows[g_cursor_chan] - g_song_page_length / 2;
        g_pattern_scroll     = player_pattern_rows[g_cursor_chan] - pattern_page_length / 2;
        g_cursor_pattern_row = player_pattern_rows[g_cursor_chan];
        g_cursor_song_row    = -1;
    }
    g_song_scroll    = clamp(g_song_scroll, 0, max_song_len - g_song_page_length);
    g_pattern_scroll = clamp(g_pattern_scroll, 0, max_pattern_len - pattern_page_length);



    gui::DrawContext& dc = gui::draw_context();


    // put text in the center of the row
    ivec2 text_offset = { 4, (g_row_height - 7) / 2 };
    char line[16];

    // song table
    for (int i = 0; i < g_song_page_length; ++i) {
        int row = g_song_scroll + i;

        gui::item_size({ 28, g_row_height });
        gui::Box box = gui::item_box();
        dc.color(color::BLACK);
        dc.fill(box);

        sprintf(line, "%02X", row);
        dc.color(color::ROW_NUMBER);
        dc.text(box.pos + text_offset + ivec2(4, 0), line);

        gui::item_size({ 80, g_row_height });
        for (int c = 0; c < 3; ++c) {
            gui::same_line();
            gui::Box box = gui::item_box();

            if (row > song.songlen[SONG_NR][c]) continue; //  include LOOP
            uint8_t v = song.songorder[SONG_NR][c][row];

            dc.color(color::BACKGROUND_ROW);
            if (v == g_pattern_nums[c]) dc.color(color::HIGHLIGHT_ROW);
            if (is_playing && row == player_song_rows[c]) dc.color(color::PLAYER_ROW);
            dc.fill(box);

            gui::ButtonState state = gui::button_state(box);
            if (state == gui::ButtonState::Released) {
                g_follow = false;
                if (v < gt::MAX_PATT) {
                    g_pattern_nums[c] = v;
                }
                g_cursor_chan        = c;
                g_cursor_pattern_row = -1;
                g_cursor_song_row    = row;
            }
            if (gui::hold()) {
                LOGD("hold");
            }

            if (state != gui::ButtonState::Normal) {
                dc.color(color::BUTTON_HELD);
                dc.box(box, gui::BoxStyle::Cursor);
            }
            if (c == g_cursor_chan && row == g_cursor_song_row) {
                dc.color(color::BUTTON_ACTIVE);
                dc.box(box, gui::BoxStyle::Cursor);
            }

            if (v == gt::LOOPSONG) {
                sprintf(line, "LOOP %02X", song.songorder[SONG_NR][c][row + 1]);
            }
            else if (v >= gt::TRANSUP) {
                sprintf(line, "TRANSP +%X", v & 0xf);
            }
            else if (v >= gt::TRANSDOWN) {
                sprintf(line, "TRANSP -%X", 16 - (v & 0xf));
            }
            else if (v >= gt::REPEAT) {
                sprintf(line, "REPEAT %X", v & 0xf);
            }
            else {
                sprintf(line, "%02X", v);
            }

            dc.color(color::WHITE);
            dc.text(box.pos + text_offset, line);
        }

    }


    // pattern bar
    gui::item_size({ 28, g_row_height });
    gui::item_box();
    gui::item_size({ 80, 16 });
    for (int c = 0; c < 3; ++c) {
        gui::same_line();

        ivec2 p = gui::cursor();
        bool active = app::player().is_channel_active(c);
        if (gui::button("", active)) {
            app::player().set_channel_active(c, !active);
        }

        sprintf(line, "%02X", g_pattern_nums[c]);
        dc.color(color::WHITE);
        dc.text(p + 4, line);

        dc.color(color::BLACK);
        dc.fill({ p + ivec2(24, 5), { 51, 6 } });

        dc.color(color::WHITE);
        dc.fill({ p + ivec2(24, 5), ivec2(sid::chan_level(c) * 51.0f + 0.9f,  6) });
    }


    // patterns
    for (int i = 0; i < pattern_page_length; ++i) {
        int row = g_pattern_scroll + i;

        gui::item_size({ 28, g_row_height });
        gui::Box box = gui::item_box();
        dc.color(color::BLACK);
        dc.fill(box);

        sprintf(line, "%02X", row);
        dc.color(color::ROW_NUMBER);
        dc.text(box.pos + text_offset + ivec2(4, 0), line);

        gui::item_size({ 80, g_row_height });
        for (int c = 0; c < 3; ++c) {
            gui::same_line();
            gui::Box box = gui::item_box();

            if (row >= song.pattlen[g_pattern_nums[c]]) continue;

            dc.color(color::BACKGROUND_ROW);
            if (row % g_row_highlight_step == 0) dc.color(color::HIGHLIGHT_ROW);
            if (is_playing && g_pattern_nums[c] == player_pattern_nums[c] && row == player_pattern_rows[c]) {
                dc.color(color::PLAYER_ROW);
            }
            dc.fill(box);

            gui::ButtonState state = gui::button_state(box);
            if (state == gui::ButtonState::Released) {
                g_follow = false;
                g_cursor_chan        = c;
                g_cursor_pattern_row = row;
                g_cursor_song_row    = -1;
            }
            if (state != gui::ButtonState::Normal) {
                dc.color(color::BUTTON_HELD);
                dc.box(box, gui::BoxStyle::Cursor);
            }
            if (c == g_cursor_chan && row == g_cursor_pattern_row) {
                dc.color(color::BUTTON_ACTIVE);
                dc.box(box, gui::BoxStyle::Cursor);
            }


            uint8_t const* p = song.pattern[g_pattern_nums[c]] + row * 4;
            int note  = p[0];
            int instr = p[1];
            int cmd   = p[2];
            int arg   = p[3];

            dc.color(color::WHITE);
            if (note == gt::REST) {
                dc.color(color::DARK_GREY);
                dc.text(box.pos + text_offset, "...");
            }
            else if (note == gt::KEYOFF) {
                dc.text(box.pos + text_offset, "===");
            }
            else if (note == gt::KEYON) {
                dc.text(box.pos + text_offset, "+++");
            }
            else {
                sprintf(line, "%c%c%d", "CCDDEFFGGAAB"[note % 12],
                                "-#-#--#-#-#-"[note % 12],
                                (note - gt::FIRSTNOTE) / 12);
                dc.text(box.pos + text_offset, line);
            }
            box.pos.x += 28;

            if (instr > 0) {
                sprintf(line, "%02X", instr);
                dc.color(color::INSTRUMENT);
                dc.text(box.pos + text_offset, line);
            }
            box.pos.x += 20;

            if (cmd > 0) {
                dc.color(color::CMDS[cmd]);
                sprintf(line, "%X%02X", cmd, arg);
                dc.text(box.pos + text_offset, line);
            }
        }
    }


    // scroll bars
    gui::cursor({ 268, 24 });
    {
        int page = g_song_page_length;
        int max_scroll = std::max<int>(0, max_song_len - page);
        gui::item_size({16, page * g_row_height});
        if (gui::vertical_drag_bar(g_song_scroll, 0, max_scroll, page)) {
            if (is_playing) g_follow = false;
        }
    }
    {
        gui::item_size(16);
        static int dy = 0;
        if (gui::vertical_drag_button(dy)) {
            g_song_page_length += dy;
        }
    }
    {
        int page = pattern_page_length;
        int max_scroll = std::max<int>(0, max_pattern_len - page);
        gui::item_size({16, page * g_row_height});
        if (gui::vertical_drag_bar(g_pattern_scroll, 0, max_scroll, page)) {
            if (is_playing) g_follow = false;
        }
    }


    // piano
    bool note_on = draw_piano();



    gui::cursor({ 0, app::canvas_height() - 48 - 16 - 24 });
    gui::align(gui::Align::Center);

    // record button
    gui::same_line();
    gui::item_size(24);
    if (gui::button(gui::Icon::Record, g_recording)) {
        g_recording = !g_recording;
    }

    gui::same_line();
    gui::cursor(gui::cursor() + ivec2(24, 0));

    // order edit
    if (g_cursor_song_row >= 0) {
        auto& order = song.songorder[SONG_NR][g_cursor_chan];
        int&  len   = song.songlen[SONG_NR][g_cursor_chan];
        int&  pos   = g_cursor_song_row;

        gui::same_line();
        if (gui::button(gui::Icon::AddRowAbove)) {
            if (len < gt::MAX_SONGLEN && pos <= len) {
                memmove(order + pos + 1, order + pos, len - pos + 2);
                order[pos] = 0;
                ++len;
                ++pos;
            }
        }
        gui::same_line();
        if (gui::button(gui::Icon::AddRowBelow)) {
            if (len < gt::MAX_SONGLEN && pos < len) {
                memmove(order + pos + 2, order + pos + 1, len - pos + 1);
                order[pos + 1] = 0;
                ++len;
            }
        }
        gui::same_line();
        if (gui::button(gui::Icon::DeleteRow)) {
            if (len > 0 && pos < len) {
                memmove(order + pos, order + pos + 1, len - pos + 1);
                order[len + 1] = 0;
                // if (order[len] > pos) --order[len];
                --len;
            }
        }

        gui::same_line();
        if (gui::button(gui::Icon::Pen)) {
            if (pos < len) {
                // TODO: order dialog
                // + pattern
                // + transpose
                // + repeat
                g_dialog = Dialog::OrderEdit;
            }
            else {
                // TODO: loop dialog

            }
        }

    }

    // pattern edit
    if (g_cursor_pattern_row >= 0) {
        int      pat_nr  = g_pattern_nums[g_cursor_chan];
        auto&    pattern = song.pattern[pat_nr];
        int&     len     = song.pattlen[pat_nr];
        int&     pos     = g_cursor_pattern_row;
        uint8_t* p       = pattern + pos * 4;

        if (g_recording && note_on) {
            p[0] = gt::FIRSTNOTE + g_piano_note;
            p[1] = g_instrument;
        }


        gui::same_line();
        if (gui::button(gui::Icon::AddRowAbove)) {
            if (len < gt::MAX_PATTROWS) {
                ++len;
                memmove(pattern + pos*4 + 4, pattern + pos*4, (len-pos) * 4);
                pattern[pos*4+0] = gt::REST;
                pattern[pos*4+1] = 0;
                pattern[pos*4+2] = 0;
                pattern[pos*4+3] = 0;
                ++pos;
            }
        }
        gui::same_line();
        if (gui::button(gui::Icon::AddRowBelow)) {
            if (len < gt::MAX_PATTROWS) {
                ++len;
                memmove(pattern + pos*4 + 8, pattern + pos*4 + 4, (len-pos) * 4 - 4);
                pattern[pos*4+4] = gt::REST;
                pattern[pos*4+5] = 0;
                pattern[pos*4+6] = 0;
                pattern[pos*4+7] = 0;
            }
        }
        gui::same_line();
        if (gui::button(gui::Icon::DeleteRow)) {
            if (len > 1) {
                --len;
                memmove(pattern + pos*4, pattern + pos*4 + 4, (len-pos) * 4 + 4);
                if (pos >= len) pos = len - 1;
            }
        }

        gui::same_line();
        if (gui::button("=")) {
            p[0] = p[0] == gt::KEYOFF ? gt::KEYON : gt::KEYOFF;
            p[1] = 0;
        }
        gui::same_line();
        if (gui::button(".")) {
            p[0] = gt::REST;
            p[1] = 0;
        }

    }




    // dialogs

    if (g_dialog == Dialog::OrderEdit) {
        gui::begin_window();

        ivec2 size = { 13 * 24, 16 * 24 + 24 };
        ivec2 pos  = ivec2(app::CANVAS_WIDTH, app::canvas_height()) / 2 - size / 2;
        dc.color(color::BLACK);
        dc.fill({ pos, size });
        gui::cursor(pos);

        uint8_t& v = song.songorder[SONG_NR][g_cursor_chan][g_cursor_song_row];

        gui::item_size(24);
        for (int i = 0; i < gt::MAX_PATT; ++i) {
            gui::same_line(i % 13 != 0);
            int n = i % 13 * 16 + i / 13;
            char str[3] = {};
            sprintf(str, "%02X", n);
            if (gui::button(str, v == n)) {
                g_dialog = Dialog::None;
                v = n;
            }
        }

        gui::item_size({ size.x, 24 });
        if (gui::button("CANCEL")) {
            g_dialog = Dialog::None;
        }

        gui::end_window();
    }

}

} // namespace song_view
