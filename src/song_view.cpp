#include "song_view.hpp"
#include "gtplayer.hpp"
#include "log.hpp"
#include "gui.hpp"
#include "app.hpp"
#include "piano.hpp"
#include "sid.hpp"
#include <array>
#include <cstring>
#include <cassert>



namespace song_view {
namespace {


enum {
    SONG_NR = 0,
};

std::array<int, 3> g_pattern_nums       = { 0, 1, 2 };
int                g_song_page          = 8;
int                g_song_scroll        = 0;
int                g_pattern_scroll     = 0;
bool               g_follow             = false;
bool               g_recording          = true;
int                g_cursor_chan        = 0;
int                g_cursor_pattern_row = 0;
int                g_cursor_song_row    = -1;


enum class Dialog {
    None,
    OrderEdit,
};
Dialog g_dialog = Dialog::None;


int                            g_transpose = 0;
int                            g_repeat    = 2;
std::array<bool, gt::MAX_PATT> g_pattern_empty;

void init_order_edit() {
    g_dialog = Dialog::OrderEdit;

    gt::Song const& song = app::song();

    uint8_t v = song.songorder[SONG_NR][g_cursor_chan][g_cursor_song_row];
    assert(v != gt::LOOPSONG);
    if      (v >= gt::TRANSUP)   g_transpose = v & 0x0f;
    else if (v >= gt::TRANSDOWN) g_transpose = (v & 0x0f) - 16;
    else if (v >= gt::REPEAT)    g_repeat = v & 0xf;

    for (int i = 0; i < gt::MAX_PATT; ++i) {
        g_pattern_empty[i] = true;
        auto const& pattern = song.pattern[i];
        int         len     = song.pattlen[i];
        for (int r = 0; r < len; ++r) {
            uint8_t const* p = pattern + r * 4;
            if (p[0] || p[1] || p[2]) {
                g_pattern_empty[i] = false;
                break;
            }
        }
    }
}

void draw_order_edit() {
    if (g_dialog != Dialog::OrderEdit) return;

    gui::Box box = gui::begin_window({ 13 * 26, (16 + 4) * app::BUTTON_WIDTH });

    gt::Song& song  = app::song();
    auto&     order = song.songorder[SONG_NR][g_cursor_chan];
    int       len   = song.songlen[SONG_NR][g_cursor_chan];
    uint8_t&  v     = order[g_cursor_song_row];

    char str[32];
    gui::item_size({ 26, app::BUTTON_WIDTH });
    for (int i = 0; i < gt::MAX_PATT; ++i) {
        gui::same_line(i % 13 != 0);
        int n = i % 13 * 16 + i / 13;
        sprintf(str, "%02X", n);
        if (gui::button(str, v == n)) {
            g_dialog = Dialog::None;
            v = n;
        }
    }

    bool is_transpose = false;
    bool is_repeat    = false;
    if      (v >= gt::TRANSUP)   is_transpose = true;
    else if (v >= gt::TRANSDOWN) is_transpose = true;
    else if (v >= gt::REPEAT)    is_repeat    = true;

    gui::item_size({ 26 * 3, app::BUTTON_WIDTH });
    sprintf(str, "TRANS %c%X", "+-"[g_transpose < 0], abs(g_transpose));
    if (gui::button(str, is_transpose)) {
        g_dialog = Dialog::None;
        if (g_transpose < 0) v = gt::TRANSDOWN | (16 + g_transpose);
        else                 v = gt::TRANSUP | g_transpose;
    }
    gui::same_line();
    gui::item_size({ 26 * 10, app::BUTTON_WIDTH });
    gui::drag_bar_style(gui::DragBarStyle::Normal);
    gui::horizontal_drag_bar(g_transpose, -0xf, 0xe, 0);

    gui::item_size({ 26 * 3, app::BUTTON_WIDTH });
    sprintf(str, "REPEAT %X", g_repeat);
    if (gui::button(str, is_repeat)) {
        g_dialog = Dialog::None;
        v = gt::REPEAT | g_repeat;
    }
    gui::same_line();
    gui::item_size({ 26 * 10, app::BUTTON_WIDTH });
    gui::horizontal_drag_bar(g_repeat, 0, 0xf, 0);

    gui::item_size({ box.size.x, app::BUTTON_WIDTH });
    if (gui::button("SET LOOP POINTER", order[len + 1] == g_cursor_song_row)) {
        g_dialog = Dialog::None;
        order[len + 1] = g_cursor_song_row;
    }
    if (gui::button("CANCEL")) {
        g_dialog = Dialog::None;
    }

    gui::end_window();
}



} // namespace


int channel() {
    return g_cursor_chan;
}


void draw() {

    gt::Player& player = app::player();
    gt::Song&   song   = app::song();
    app::Settings const& settings = app::settings();

    gui::DrawContext& dc = gui::draw_context();

    bool is_playing = player.is_playing();


    // get player position info
    std::array<int, 3> player_song_rows;
    std::array<int, 3> player_pattern_nums;
    std::array<int, 3> player_pattern_rows;
    for (int c = 0; c < 3; ++c) {
        uint8_t  songptr;
        uint8_t  pattnum;
        uint32_t pattptr;
        player.get_chan_info(c, songptr, pattnum, pattptr);
        player_song_rows[c]    = std::max<int>(0, songptr - 1);
        player_pattern_nums[c] = pattnum;
        player_pattern_rows[c] = pattptr / 4;
    }

    int max_song_len =                    song.songlen[SONG_NR][0];
    max_song_len = std::max(max_song_len, song.songlen[SONG_NR][1]);
    max_song_len = std::max(max_song_len, song.songlen[SONG_NR][2]);
    max_song_len += 1; // include LOOP command

    int max_pattern_len =                       song.pattlen[g_pattern_nums[0]];
    max_pattern_len = std::max(max_pattern_len, song.pattlen[g_pattern_nums[1]]);
    max_pattern_len = std::max(max_pattern_len, song.pattlen[g_pattern_nums[2]]);


    gui::cursor({ 0, 24 });

    int height_left = app::canvas_height() - (gui::cursor().y + 48 + 20 + 20);
    int total_rows = height_left / settings.row_height;
    g_song_page = clamp(g_song_page, 0, total_rows);
    int pattern_page = total_rows - g_song_page;



    if (is_playing && g_follow) {
        // auto scroll
        g_pattern_nums       = player_pattern_nums;
        g_song_scroll        = player_song_rows[g_cursor_chan] - g_song_page / 2;
        g_pattern_scroll     = player_pattern_rows[g_cursor_chan] - pattern_page / 2;
        g_cursor_pattern_row = player_pattern_rows[g_cursor_chan];
        g_cursor_song_row    = -1;
    }
    else {
        if (g_cursor_pattern_row >= 0) {
            assert(g_cursor_song_row == -1);
            int len = song.pattlen[g_cursor_chan];
            if (g_cursor_pattern_row >= len) g_cursor_pattern_row = -1;
        }
        if (g_cursor_song_row >= 0) {
            assert(g_cursor_pattern_row == -1);
            int len = song.songlen[SONG_NR][g_cursor_chan];
            if (g_cursor_song_row > len) g_cursor_song_row = -1;
        }
    }
    g_song_scroll    = clamp(g_song_scroll, 0, max_song_len - g_song_page);
    g_pattern_scroll = clamp(g_pattern_scroll, 0, max_pattern_len - pattern_page);



    // put text in the center of the row
    int text_offset = (settings.row_height - 7) / 2;
    char str[32];

    // song table
    for (int i = 0; i < g_song_page; ++i) {
        int row = g_song_scroll + i;

        gui::item_size({ 28, settings.row_height });
        gui::Box box = gui::item_box();

        sprintf(str, "%02X", row);
        dc.color(color::ROW_NUMBER);
        dc.text(box.pos + ivec2(6, text_offset), str);

        gui::item_size({ 84, settings.row_height });
        for (int c = 0; c < 3; ++c) {
            gui::same_line();
            gui::Box box = gui::item_box();
            gui::ButtonState state = gui::button_state(box);
            box.pos.x += 1;
            box.size.x -= 2;

            auto const& order = song.songorder[SONG_NR][c];
            int         len   = song.songlen[SONG_NR][c];

            if (row >= len) continue;
            uint8_t v = order[row];

            dc.color(color::BACKGROUND_ROW);
            if (v == g_pattern_nums[c]) dc.color(color::HIGHLIGHT_ROW);
            if (is_playing && row == player_song_rows[c]) dc.color(color::PLAYER_ROW);
            dc.fill(box);

            if (state == gui::ButtonState::Released) {
                g_follow = false;
                if (v < gt::MAX_PATT) {
                    g_pattern_nums[c] = v;
                }
                g_cursor_chan        = c;
                g_cursor_pattern_row = -1;
                g_cursor_song_row    = row;
            }

            if (state != gui::ButtonState::Normal) {
                dc.color(color::BUTTON_HELD);
                dc.box(box, gui::BoxStyle::Cursor);
            }
            if (c == g_cursor_chan && row == g_cursor_song_row) {
                dc.color(color::BUTTON_ACTIVE);
                dc.box(box, gui::BoxStyle::Cursor);
            }

            assert(v != gt::LOOPSONG);
            if (v >= gt::TRANSUP) {
                sprintf(str, "TRANS +%X", v & 0xf);
            }
            else if (v >= gt::TRANSDOWN) {
                sprintf(str, "TRANS -%X", 16 - (v & 0xf));
            }
            else if (v >= gt::REPEAT) {
                sprintf(str, "REPEAT %X", v & 0xf);
            }
            else {
                sprintf(str, "%02X", v);
            }
            dc.color(color::WHITE);
            dc.text(box.pos + ivec2(5, text_offset), str);

            // loop marker
            if (row == order[len + 1]) {
                dc.color(color::BUTTON_HELD);
                dc.text(box.pos + ivec2(box.size.x - 8, text_offset), "\x05");
            }
        }

    }


    // pattern bar
    gui::item_size({ 28, settings.row_height });
    gui::item_box();
    gui::item_size({ 84, 20 });
    for (int c = 0; c < 3; ++c) {
        gui::same_line();
        ivec2 p = gui::cursor();
        bool active = player.is_channel_active(c);
        sprintf(str, "%02X", g_pattern_nums[c]);
        gui::align(gui::Align::Left);
        if (gui::button(str, active)) {
            player.set_channel_active(c, !active);
        }
        dc.color(color::BLACK);
        dc.fill({ p + ivec2(27, 7), { 50, 6 } });
        dc.color(color::WHITE);
        dc.fill({ p + ivec2(27, 7), ivec2(sid::chan_level(c) * 50.0f + 0.9f, 6) });
    }


    // patterns
    for (int i = 0; i < pattern_page; ++i) {
        int row = g_pattern_scroll + i;

        gui::item_size({ 28, settings.row_height });
        gui::Box box = gui::item_box();

        sprintf(str, "%02X", row);
        dc.color(color::ROW_NUMBER);
        dc.text(box.pos + ivec2(6, text_offset), str);

        gui::item_size({ 84, settings.row_height });
        for (int c = 0; c < 3; ++c) {
            gui::same_line();
            gui::Box box = gui::item_box();
            gui::ButtonState state = gui::button_state(box);
            box.pos.x += 1;
            box.size.x -= 2;

            if (row >= song.pattlen[g_pattern_nums[c]]) continue;

            dc.color(color::BACKGROUND_ROW);
            if (row % settings.row_highlight == 0) dc.color(color::HIGHLIGHT_ROW);
            if (is_playing && g_pattern_nums[c] == player_pattern_nums[c] && row == player_pattern_rows[c]) {
                dc.color(color::PLAYER_ROW);
            }
            dc.fill(box);

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

            ivec2 t = box.pos + ivec2(5, text_offset);

            dc.color(color::WHITE);
            if (note == gt::REST) {
                dc.color(color::DARK_GREY);
                dc.text(t, "\x01\x01\x01");
            }
            else if (note == gt::KEYOFF) {
                dc.text(t, "\x02\x02\x02");
            }
            else if (note == gt::KEYON) {
                dc.text(t, "\x03\x03\x03");
            }
            else {
                sprintf(str, "%c%c%d", "CCDDEFFGGAAB"[note % 12],
                                "-#-#--#-#-#-"[note % 12],
                                (note - gt::FIRSTNOTE) / 12);
                dc.text(t, str);
            }
            t.x += 28;

            if (instr > 0) {
                sprintf(str, "%02X", instr);
                dc.color(color::INSTRUMENT);
                dc.text(t, str);
            }
            t.x += 20;

            if (cmd > 0) {
                dc.color(color::CMDS[cmd]);
                sprintf(str, "%X%02X", cmd, arg);
                dc.text(t, str);
            }
        }
    }


    // scroll bars
    gui::cursor({ app::CANVAS_WIDTH - 80, 24 });
    gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
    {
        int page = g_song_page;
        int max_scroll = std::max<int>(0, max_song_len - page);
        gui::item_size({ app::BUTTON_WIDTH, page * settings.row_height });
        if (gui::vertical_drag_bar(g_song_scroll, 0, max_scroll, page)) {
            if (is_playing) g_follow = false;
        }
    }
    {
        gui::item_size(app::BUTTON_WIDTH);
        static int dy = 0;
        if (gui::vertical_drag_button(dy)) {
            g_song_page += dy;
        }
    }
    {
        int page = pattern_page;
        int max_scroll = std::max<int>(0, max_pattern_len - page);
        gui::item_size({ app::BUTTON_WIDTH, page * settings.row_height });
        if (gui::vertical_drag_bar(g_pattern_scroll, 0, max_scroll, page)) {
            if (is_playing) g_follow = false;
        }
    }


    // piano
    if (piano::draw() && g_recording) {
        int      pat_nr  = g_pattern_nums[g_cursor_chan];
        auto&    pattern = song.pattern[pat_nr];
        uint8_t* p       = pattern + g_cursor_pattern_row * 4;
        p[0] = gt::FIRSTNOTE + piano::note();
        p[1] = piano::instrument();
    }

    // buttons
    gui::cursor({ app::CANVAS_WIDTH - 60, 24 });
    gui::align(gui::Align::Center);
    gui::item_size({ 60, app::BUTTON_WIDTH });

    if (gui::button(gui::Icon::Play, is_playing)) {
        if (is_playing) {
            player.stop_song();
        }
        else {
            player.init_song(0, gt::Player::PLAY_BEGINNING);
        }
    }

    if (gui::button(gui::Icon::Follow, g_follow)) {
        g_follow = !g_follow;
    }

    // pattern edit
    if (g_cursor_pattern_row >= 0 && !(is_playing && g_follow)) {
        int      pat_nr  = g_pattern_nums[g_cursor_chan];
        auto&    pattern = song.pattern[pat_nr];
        int&     len     = song.pattlen[pat_nr];
        int&     pos     = g_cursor_pattern_row;
        uint8_t* p       = pattern + pos * 4;

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
        if (gui::button(gui::Icon::DeleteRow)) {
            if (len > 1) {
                --len;
                memmove(pattern + pos*4, pattern + pos*4 + 4, (len-pos) * 4 + 4);
                if (pos >= len) pos = len - 1;
            }
        }

        if (gui::button("\x01\x01\x01")) {
            p[0] = gt::REST;
            p[1] = 0;
        }
        if (gui::button("\x02\x02\x02")) {
            p[0] = p[0] == gt::KEYOFF ? gt::KEYON : gt::KEYOFF;
            p[1] = 0;
        }
    }

    // order edit
    if (g_cursor_song_row >= 0 && !(is_playing && g_follow)) {
        auto& order = song.songorder[SONG_NR][g_cursor_chan];
        int&  len   = song.songlen[SONG_NR][g_cursor_chan];
        int&  pos   = g_cursor_song_row;

        if (gui::button(gui::Icon::AddRowAbove)) {
            if (len < gt::MAX_SONGLEN && pos <= len) {
                memmove(order + pos + 1, order + pos, len - pos + 2);
                order[pos] = 0;
                ++len;
                ++pos;
            }
        }
        if (gui::button(gui::Icon::AddRowBelow)) {
            if (len < gt::MAX_SONGLEN && pos < len) {
                memmove(order + pos + 2, order + pos + 1, len - pos + 1);
                order[pos + 1] = 0;
                ++len;
            }
        }
        if (gui::button(gui::Icon::DeleteRow)) {
            if (len > 1 && pos < len) {
                memmove(order + pos, order + pos + 1, len - pos + 1);
                order[len + 1] = 0;
                // if (order[len] > pos) --order[len];
                --len;
            }
        }

        if (gui::button(gui::Icon::Pen)) {
            if (pos < len) {
                init_order_edit();
            }
            else {
                // TODO: loop dialog
            }
        }

    }



    // record button
    gui::cursor({ app::CANVAS_WIDTH - 60, app::canvas_height() - 48 - app::BUTTON_WIDTH - 20 });
    if (gui::button(gui::Icon::Record, g_recording)) {
        g_recording = !g_recording;
    }


    draw_order_edit();
}

} // namespace song_view
