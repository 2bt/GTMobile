#include "song_view.hpp"
#include "settings_view.hpp"
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

enum class Dialog { None, OrderEdit };
enum class EditMode { Song, Pattern };

int      g_song_page          = 8;
int      g_song_scroll        = 0;
int      g_pattern_scroll     = 0;
bool     g_follow             = false;
bool     g_recording          = false;
int      g_cursor_chan        = 0;
int      g_cursor_pattern_row = 0;
int      g_cursor_song_row    = 0;
int      g_transpose          = 0;
EditMode g_edit_mode          = EditMode::Song;
Dialog   g_dialog             = Dialog::None;

std::array<bool, gt::MAX_PATT> g_pattern_empty;

void init_order_edit() {
    g_dialog = Dialog::OrderEdit;
    gt::Song const& song = app::song();
    g_transpose = song.song_order[g_cursor_chan][g_cursor_song_row].trans;
    for (int i = 0; i < gt::MAX_PATT; ++i) {
        g_pattern_empty[i] = true;
        gt::Pattern const& patt = song.patterns[i];
        for (int r = 0; r < patt.len; ++r) {
            auto row = patt.rows[r];
            if (row.note != gt::REST || row.instr || row.command) {
                g_pattern_empty[i] = false;
                break;
            }
        }
    }
}

void exit_order_edit() {
    g_dialog = Dialog::None;
    // change transpose on all rows below
    gt::Song& song  = app::song();
    auto&     order = song.song_order[g_cursor_chan];
    int trans = order[g_cursor_song_row].trans;
    for (int i = g_cursor_song_row; i < song.song_len; ++i) {
        if (order[i].trans != trans) break;
        order[i].trans = g_transpose;
    }
}

void draw_order_edit() {
    if (g_dialog != Dialog::OrderEdit) return;

    gui::Box box = gui::begin_window({ 13 * 26, (16 + 3) * app::BUTTON_HEIGHT });
    gui::item_size({ box.size.x, app::BUTTON_HEIGHT });
    gui::text("EDIT ORDER LIST");

    gt::Song&     song  = app::song();
    auto&         order = song.song_order[g_cursor_chan];
    gt::OrderRow& row   = order[g_cursor_song_row];

    char str[32];
    gui::item_size({ 26, app::BUTTON_HEIGHT });
    for (int i = 0; i < gt::MAX_PATT; ++i) {
        gui::same_line(i % 13 != 0);
        int n = i % 13 * 16 + i / 13;
        sprintf(str, "%02X", n);
        if (!g_pattern_empty[n]) gui::button_style(gui::ButtonStyle::Tagged);
        if (gui::button(str, row.pattnum == n)) {
            row.pattnum = n;
            exit_order_edit();
        }
        gui::button_style(gui::ButtonStyle::Normal);
    }

    gui::item_size({ 26 * 4, app::BUTTON_HEIGHT });
    sprintf(str, "TRANSPOSE %c%X", "+-"[g_transpose < 0], abs(g_transpose));
    gui::text(str);
    gui::same_line();
    gui::item_size({ 26 * 9, app::BUTTON_HEIGHT });
    gui::drag_bar_style(gui::DragBarStyle::Normal);
    gui::horizontal_drag_bar(g_transpose, -0xf, 0xe, 0);

    gui::item_size({ box.size.x, app::BUTTON_HEIGHT });
    if (gui::button("CLOSE")) {
        exit_order_edit();
    }

    gui::end_window();
}


} // namespace


int channel() { return g_cursor_chan; }

int song_position() {
    gt::Song& song = app::song();
    if (g_cursor_song_row >= song.song_len) {
        g_cursor_song_row = song.song_len - 1;
    }
    return g_cursor_song_row;
}

void reset() {
    g_cursor_pattern_row = 0;
    g_cursor_song_row    = 0;
    g_song_scroll        = 0;
    g_pattern_scroll     = 0;
    g_follow             = false;
    g_recording          = false;
    g_edit_mode          = EditMode::Song;
}


void draw() {

    gt::Player& player = app::player();
    gt::Song&   song   = app::song();
    settings_view::Settings const& settings = settings_view::settings();
    gui::DrawContext& dc = gui::draw_context();

    // get player position info
    std::array<int, 3> player_song_rows = player.m_current_song_pos;
    std::array<int, 3> player_patt_rows = player.m_current_patt_pos;
    std::array<int, 3> player_patt_nums;
    for (int c = 0; c < 3; ++c) {
        player_patt_nums[c] = song.song_order[c][player_song_rows[c]].pattnum;
    }

    enum {
        MAX_SONG_ROWS = 127, // --> gt::MAX_SONGLEN / 2
    };

    gui::cursor({ 0, app::TAB_HEIGHT });
    int height_left = app::canvas_height() - gui::cursor().y - piano::HEIGHT - app::BUTTON_HEIGHT;
    int total_rows = height_left / settings.row_height;
    g_song_page = clamp(g_song_page, 0, total_rows);
    int pattern_page = total_rows - g_song_page;

    bool is_playing = player.is_playing();
    std::array<int, 3> patt_nums;
    if (g_follow) {
        // auto scroll
        g_edit_mode          = EditMode::Pattern;
        g_cursor_song_row    = player_song_rows[g_cursor_chan];
        g_cursor_pattern_row = player_patt_rows[g_cursor_chan];
        g_song_scroll        = player_song_rows[g_cursor_chan] - g_song_page / 2;
        g_pattern_scroll     = player_patt_rows[g_cursor_chan] - pattern_page / 2;
        patt_nums            = player_patt_nums;
    }
    else {
        // check bounds if a new song was loaded
        if (g_cursor_song_row >= song.song_len) {
            g_cursor_song_row = song.song_len - 1;
        }
        for (int k = 0; k < gt::MAX_CHN; ++k) {
            patt_nums[k] = song.song_order[k][g_cursor_song_row].pattnum;
        }
    }

    int max_pattern_len = song.patterns[patt_nums[0]].len;
    max_pattern_len = std::max(max_pattern_len, song.patterns[patt_nums[1]].len);
    max_pattern_len = std::max(max_pattern_len, song.patterns[patt_nums[2]].len);

    g_song_scroll    = clamp(g_song_scroll, 0, song.song_len - g_song_page);
    g_pattern_scroll = clamp(g_pattern_scroll, 0, max_pattern_len - pattern_page);


    // put text in the center of the row
    int text_offset = (settings.row_height - 7) / 2;
    char str[32];

    // song table
    for (int i = 0; i < g_song_page; ++i) {
        int r = g_song_scroll + i;

        gui::item_size({ 28, settings.row_height });
        gui::Box box = gui::item_box();

        sprintf(str, "%02X", r);
        dc.rgb(color::ROW_NUMBER);
        dc.text(box.pos + ivec2(6, text_offset), str);

        if (r == song.song_loop) {
            dc.rgb(color::BUTTON_HELD);
            dc.text(box.pos + ivec2(0, text_offset), "\x04");
        }

        gui::item_size({ 84, settings.row_height });
        for (int c = 0; c < 3; ++c) {
            gui::same_line();
            gui::Box box = gui::item_box();
            gui::ButtonState state = gui::button_state(box);
            box.pos.x += 1;
            box.size.x -= 2;

            if (r >= int(song.song_len)) continue;
            gt::OrderRow& row = song.song_order[c][r];

            dc.rgb(color::BACKGROUND_ROW);
            if (row.pattnum == patt_nums[c]) dc.rgb(color::HIGHLIGHT_ROW);
            if (r == player_song_rows[c]) dc.rgb(color::PLAYER_ROW);
            dc.fill(box);

            if (state == gui::ButtonState::Released) {
                g_follow          = false;
                g_edit_mode       = EditMode::Song;
                g_cursor_chan     = c;
                g_cursor_song_row = r;
                for (int k = 0; k < gt::MAX_CHN; ++k) {
                    patt_nums[k] = song.song_order[k][r].pattnum;
                }
            }

            if (state != gui::ButtonState::Normal) {
                dc.rgb(color::BUTTON_HELD);
                dc.box(box, gui::BoxStyle::Cursor);
            }
            if (g_edit_mode == EditMode::Song && c == g_cursor_chan && r == g_cursor_song_row) {
                dc.rgb(color::BUTTON_ACTIVE);
                dc.box(box, gui::BoxStyle::Cursor);
            }

            int prev_trans = r == 0 ? 0 : song.song_order[c][r - 1].trans;
            if (row.trans >= 0) sprintf(str, "+%X", row.trans);
            else                sprintf(str, "-%X", -row.trans);
            dc.rgb(color::WHITE);
            if (row.trans == prev_trans)  dc.rgb(color::DARK_GREY);
            dc.text(box.pos + ivec2(5, text_offset), str);
            sprintf(str, "   %02X", row.pattnum);
            dc.rgb(color::WHITE);
            dc.text(box.pos + ivec2(5, text_offset), str);
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
        sprintf(str, "%02X", patt_nums[c]);
        gui::align(gui::Align::Left);
        if (gui::button(str, active)) {
            player.set_channel_active(c, !active);
        }
        dc.rgb(color::BLACK);
        dc.fill({ p + ivec2(27, 7), { 50, 6 } });
        dc.rgb(color::WHITE);
        dc.fill({ p + ivec2(27, 7), ivec2(sid::chan_level(c) * 50.0f + 0.9f, 6) });
    }


    // patterns
    for (int i = 0; i < pattern_page; ++i) {
        int r = g_pattern_scroll + i;

        gui::item_size({ 28, settings.row_height });
        gui::Box box = gui::item_box();

        sprintf(str, "%02X", r);
        dc.rgb(color::ROW_NUMBER);
        dc.text(box.pos + ivec2(6, text_offset), str);

        gui::item_size({ 84, settings.row_height });
        for (int c = 0; c < 3; ++c) {
            gui::same_line();
            gui::Box box = gui::item_box();
            gt::Pattern& patt = song.patterns[patt_nums[c]];
            if (r >= patt.len) continue;

            gui::ButtonState state = gui::button_state(box);
            box.pos.x += 1;
            box.size.x -= 2;
            dc.rgb(color::BACKGROUND_ROW);
            if (r % settings.row_highlight == 0) dc.rgb(color::HIGHLIGHT_ROW);
            if (patt_nums[c] == player_patt_nums[c] && r == player_patt_rows[c]) {
                dc.rgb(color::PLAYER_ROW);
            }
            dc.fill(box);

            if (state == gui::ButtonState::Released) {
                g_edit_mode          = EditMode::Pattern;
                g_follow             = false;
                g_cursor_chan        = c;
                g_cursor_pattern_row = r;
            }
            if (state != gui::ButtonState::Normal) {
                dc.rgb(color::BUTTON_HELD);
                dc.box(box, gui::BoxStyle::Cursor);
            }
            if (g_edit_mode == EditMode::Pattern && c == g_cursor_chan && r == g_cursor_pattern_row) {
                dc.rgb(color::BUTTON_ACTIVE);
                dc.box(box, gui::BoxStyle::Cursor);
            }

            gt::PatternRow& row = patt.rows[r];
            ivec2 t = box.pos + ivec2(5, text_offset);
            dc.rgb(color::WHITE);
            if (row.note == gt::REST) {
                dc.rgb(color::DARK_GREY);
                dc.text(t, "\x01\x01\x01");
            }
            else if (row.note == gt::KEYOFF) {
                dc.text(t, "\x02\x02\x02");
            }
            else if (row.note == gt::KEYON) {
                dc.text(t, "\x03\x03\x03");
            }
            else {
                sprintf(str, "%c%c%d", "CCDDEFFGGAAB"[row.note % 12],
                             "-#-#--#-#-#-"[row.note % 12],
                             (row.note - gt::FIRSTNOTE) / 12);
                dc.text(t, str);
            }
            t.x += 28;

            if (row.instr > 0) {
                sprintf(str, "%02X", row.instr);
                dc.rgb(color::INSTRUMENT);
                dc.text(t, str);
            }
            t.x += 20;

            if (row.command > 0) {
                dc.rgb(color::CMDS[row.command]);
                sprintf(str, "%X%02X", row.command, row.data);
                dc.text(t, str);
            }
        }
    }


    // scroll bars
    gui::cursor({ app::CANVAS_WIDTH - 80, app::TAB_HEIGHT });
    gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
    {
        int page = g_song_page;
        // int max_scroll = std::max(0, max_song_len - page);
        int max_scroll = std::max(0, song.song_len - page);
        gui::item_size({ app::BUTTON_HEIGHT, page * settings.row_height });
        if (gui::vertical_drag_bar(g_song_scroll, 0, max_scroll, page)) {
            g_follow = false;
        }
    }
    {
        gui::item_size(app::BUTTON_HEIGHT);
        static int dy = 0;
        if (gui::vertical_drag_button(dy)) {
            g_song_page += dy;
        }
    }
    {
        int page = pattern_page;
        int max_scroll = std::max<int>(0, max_pattern_len - page);
        gui::item_size({ app::BUTTON_HEIGHT, page * settings.row_height });
        if (gui::vertical_drag_bar(g_pattern_scroll, 0, max_scroll, page)) {
            g_follow = false;
        }
    }


    // buttons
    gui::cursor({ app::CANVAS_WIDTH - 60, app::TAB_HEIGHT });
    gui::align(gui::Align::Center);

    // order edit
    if (g_edit_mode == EditMode::Song && !(is_playing && g_follow)) {
        int& pos = g_cursor_song_row;
        int& len = song.song_len;
        assert(pos < len);

        gui::item_size(app::TAB_HEIGHT);
        gui::disabled(!(len < MAX_SONG_ROWS && pos <= len));
        if (gui::button(gui::Icon::AddRowAbove)) {
            for (auto& order : song.song_order) {
                std::rotate(order.begin() + pos, order.end() - 1, order.end());
                order[pos] = order[pos + 1];
            }
            if (song.song_loop >= pos) ++song.song_loop;
            ++pos;
            ++len;
        }
        gui::same_line();
        if (gui::button(gui::Icon::AddRowBelow)) {
            for (auto& order : song.song_order) {
                std::rotate(order.begin() + pos + 1, order.end() - 1, order.end());
                order[pos + 1] = order[pos];
            }
            if (song.song_loop > pos) ++song.song_loop;
            ++len;
        }
        gui::disabled(!(len > 1 && pos < len));
        if (gui::button(gui::Icon::DeleteRow)) {
            for (auto& order : song.song_order) {
                std::rotate(order.begin() + pos, order.begin() + pos + 1, order.end());
            }
            --len;
            if (pos > 0 && song.song_loop > pos) --song.song_loop;
            if (song.song_loop >= len) song.song_loop = len - 1;
            if (pos >= len) --pos;
        }
        gui::same_line();
        gui::disabled(song.song_loop == g_cursor_song_row);
        if (gui::button(gui::Icon::JumpBack)) {
            song.song_loop = g_cursor_song_row;
        }
        gui::disabled(false);

        gui::item_size({ app::TAB_HEIGHT * 2, app::TAB_HEIGHT });
        if (gui::button(gui::Icon::Pen)) {
            init_order_edit();
        }


    }

    // pattern edit
    gt::Pattern& patt = song.patterns[patt_nums[g_cursor_chan]];
    if (g_edit_mode == EditMode::Pattern && g_cursor_pattern_row < patt.len && !(is_playing && g_follow)) {
        auto& rows = patt.rows;
        int&  len  = patt.len;
        int&  pos  = g_cursor_pattern_row;

        gui::item_size(app::TAB_HEIGHT);
        gui::disabled(!(len < gt::MAX_PATTROWS));
        if (gui::button(gui::Icon::AddRowAbove)) {
            std::rotate(rows.begin() + pos, rows.end() - 1, rows.end());
            rows[pos] = {};
            ++pos;
            ++len;
        }
        gui::same_line();
        if (gui::button(gui::Icon::AddRowBelow)) {
            std::rotate(rows.begin() + pos + 1, rows.end() - 1, rows.end());
            rows[pos + 1] = {};
            ++len;
        }
        gui::disabled(!(len > 1));
        if (gui::button(gui::Icon::DeleteRow)) {
            std::rotate(rows.begin() + pos, rows.begin() + pos + 1, rows.end());
            --len;
            if (pos >= len) --pos;
        }
        gui::disabled(false);

        gt::PatternRow& row  = patt.rows[pos];
        if (gui::button("\x01\x01\x01")) {
            row.note  = gt::REST;
            row.instr = 0;
        }
        gui::same_line();
        if (gui::button("\x02\x02\x02")) {
            row.note  = row.note == gt::KEYOFF ? gt::KEYON : gt::KEYOFF;
            row.instr = 0;
        }

        // record button
        gui::item_size({ app::TAB_HEIGHT * 2, app::TAB_HEIGHT });
        if (gui::button(gui::Icon::Record, g_recording)) {
            g_recording = !g_recording;
        }
    }


    draw_order_edit();

    // piano
    if (piano::draw(&g_follow) && g_edit_mode == EditMode::Pattern && g_recording) {
        assert(g_cursor_pattern_row < patt.len);
        gt::PatternRow& row  = patt.rows[g_cursor_pattern_row];
        row.note  = gt::FIRSTNOTE + piano::note();
        row.instr = piano::instrument();
    }
}

} // namespace song_view
