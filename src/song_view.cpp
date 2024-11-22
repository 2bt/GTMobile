#include "song_view.hpp"
#include "settings_view.hpp"
#include "command_edit.hpp"
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

enum class Dialog { None, OrderEdit, CommandEdit };
enum class EditMode { Song, Pattern };


gt::Song&       g_song               = app::song();
int             g_song_page          = 8;
bool            g_follow;
bool            g_recording;
EditMode        g_edit_mode;
int             g_song_scroll;
int             g_pattern_scroll;
int             g_cursor_pattern_row = 0;
int             g_cursor_song_row    = 0;
int             g_cursor_chan        = 0;
Dialog          g_dialog             = Dialog::None;
int             g_transpose          = 0;
gt::PatternRow* g_command_row        = nullptr;

std::array<bool, gt::MAX_PATT> g_pattern_empty;


void init_order_edit() {
    g_dialog = Dialog::OrderEdit;
    g_transpose = g_song.song_order[g_cursor_chan][g_cursor_song_row].trans;
    for (int i = 0; i < gt::MAX_PATT; ++i) {
        g_pattern_empty[i] = true;
        gt::Pattern const& patt = g_song.patterns[i];
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
    auto&     order = g_song.song_order[g_cursor_chan];
    int trans = order[g_cursor_song_row].trans;
    for (int i = g_cursor_song_row; i < g_song.song_len; ++i) {
        if (order[i].trans != trans) break;
        order[i].trans = g_transpose;
    }
}


void draw_order_edit() {
    enum {
        MIN_HEIGHT = 3 * app::BUTTON_HEIGHT + gui::FRAME_WIDTH * 2,
        CW = (app::CANVAS_WIDTH - 12) / 8,
        WIDTH = 8 * CW,
    };
    int space = app::canvas_height() - MIN_HEIGHT - 12;
    int row_h = std::min<int>(space / 26, app::BUTTON_HEIGHT);
    int height = MIN_HEIGHT + row_h * 26;

    gui::begin_window({ WIDTH, height });
    gui::item_size({ WIDTH, app::BUTTON_HEIGHT });
    gui::text("ORDER LIST EDIT");
    gui::separator();

    auto&         order = g_song.song_order[g_cursor_chan];
    gt::OrderRow& row   = order[g_cursor_song_row];

    char str[32];
    gui::item_size({ CW, row_h });
    for (int i = 0; i < gt::MAX_PATT; ++i) {
        gui::same_line(i % 8 != 0);
        sprintf(str, "%02X", i);
        gui::button_style(g_pattern_empty[i] ? gui::ButtonStyle::Shaded : gui::ButtonStyle::Normal);
        if (gui::button(str, row.pattnum == i)) {
            row.pattnum = i;
            exit_order_edit();
        }
    }
    gui::button_style(gui::ButtonStyle::Normal);
    gui::item_size({ WIDTH, app::BUTTON_HEIGHT });
    gui::separator();

    sprintf(str, "TRANSPOSE %c%X", "+-"[g_transpose < 0], abs(g_transpose));
    gui::slider(WIDTH, str, g_transpose, -0xf, 0xe);
    gui::item_size({ WIDTH, app::BUTTON_HEIGHT });
    if (gui::button("CLOSE")) exit_order_edit();
    gui::end_window();
}





} // namespace


int channel() { return g_cursor_chan; }

int song_position() {
    if (g_cursor_song_row >= g_song.song_len) {
        g_cursor_song_row = g_song.song_len - 1;
    }
    return g_cursor_song_row;
}

void reset() {
    g_follow             = false;
    g_recording          = false;
    g_edit_mode          = EditMode::Pattern;
    g_cursor_pattern_row = 0;
    g_cursor_song_row    = 0;
    g_song_scroll        = 0;
    g_pattern_scroll     = 0;
}


void draw() {

    gt::Player& player = app::player();
    settings_view::Settings const& settings = settings_view::settings();
    gui::DrawContext& dc = gui::draw_context();

    // get player position info
    std::array<int, 3> player_song_rows = player.m_current_song_pos;
    std::array<int, 3> player_patt_rows = player.m_current_patt_pos;
    std::array<int, 3> player_patt_nums;
    for (int c = 0; c < 3; ++c) {
        player_patt_nums[c] = g_song.song_order[c][player_song_rows[c]].pattnum;
    }

    enum {
        MAX_SONG_ROWS = 127, // --> gt::MAX_SONGLEN / 2
        CN = 28,
        CC = 84,
    };

    ivec2 cursor = gui::cursor();
    gui::cursor({ 0, cursor.y + 1 }); // 1px padding

    int pattern_page;
    {
        int height_left = app::canvas_height() - cursor.y - piano::HEIGHT - app::BUTTON_HEIGHT;
        int total_rows = (height_left - 4) / settings.row_height;
        g_song_page = clamp(g_song_page, 0, total_rows);
        pattern_page = total_rows - g_song_page;
    }

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
        if (g_cursor_song_row >= g_song.song_len) {
            g_cursor_song_row = g_song.song_len - 1;
        }
        for (int k = 0; k < gt::MAX_CHN; ++k) {
            patt_nums[k] = g_song.song_order[k][g_cursor_song_row].pattnum;
        }
    }

    int max_pattern_len = g_song.patterns[patt_nums[0]].len;
    max_pattern_len = std::max(max_pattern_len, g_song.patterns[patt_nums[1]].len);
    max_pattern_len = std::max(max_pattern_len, g_song.patterns[patt_nums[2]].len);

    g_song_scroll    = clamp(g_song_scroll, 0, g_song.song_len - g_song_page);
    g_pattern_scroll = clamp(g_pattern_scroll, 0, max_pattern_len - pattern_page);


    // put text in the center of the row
    int text_offset = (settings.row_height - 7) / 2;
    char str[32];

    // song table
    for (int i = 0; i < g_song_page; ++i) {
        int r = g_song_scroll + i;

        gui::item_size({ CN, settings.row_height });
        gui::Box box = gui::item_box();

        sprintf(str, "%02X", r);
        dc.rgb(color::ROW_NUMBER);
        dc.text(box.pos + ivec2(6, text_offset), str);

        gui::item_size({ CC, settings.row_height });
        for (int c = 0; c < 3; ++c) {
            gui::same_line();
            gui::Box box = gui::item_box();
            gui::ButtonState state = gui::button_state(box);
            box.pos.x += 1;
            box.size.x -= 2;

            if (r >= int(g_song.song_len)) continue;
            gt::OrderRow& row = g_song.song_order[c][r];

            dc.rgb(color::BACKGROUND_ROW);
            // if (row.pattnum == patt_nums[c]) dc.rgb(color::HIGHLIGHT_ROW);
            if (r == player_song_rows[c]) dc.rgb(color::PLAYER_ROW);
            else if (r == g_cursor_song_row) dc.rgb(color::HIGHLIGHT_ROW);
            dc.fill(box);

            if (state == gui::ButtonState::Released) {
                g_follow          = false;
                g_edit_mode       = EditMode::Song;
                g_cursor_chan     = c;
                g_cursor_song_row = r;
                for (int k = 0; k < gt::MAX_CHN; ++k) {
                    patt_nums[k] = g_song.song_order[k][r].pattnum;
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

            sprintf(str, "   %c%X", "+-"[row.trans < 0], abs(row.trans));
            int prev_trans = r == 0 ? 0 : g_song.song_order[c][r - 1].trans;
            dc.rgb(row.trans == prev_trans ? color::DARK_GREY : color::WHITE);
            dc.text(box.pos + ivec2(5, text_offset), str);

            sprintf(str, "%02X", row.pattnum);
            dc.rgb(color::WHITE);
            dc.text(box.pos + ivec2(5, text_offset), str);
        }

        // loop marker
        if (r == g_song.song_loop) {
            dc.rgb(color::BUTTON_HELD);
            dc.text(box.pos + ivec2(CN + CC * 3 - 9, text_offset), "\x05");
        }
    }

    gui::cursor({ 0, gui::cursor().y + 1 }); // 1px padding

    // pattern bar
    gui::item_size({ CN, settings.row_height });
    gui::item_box();
    gui::item_size({ CC, app::BUTTON_HEIGHT });
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
        dc.fill({ p + ivec2(28, 11), { 48, 8 } });
        dc.fill({ p + ivec2(27, 12), { 50, 6 } });
        dc.rgb(color::mix(color::GREEN, 0, 0.2f));
        dc.fill({ p + ivec2(29, 13), ivec2(sid::chan_level(c) * 46.0f + 0.9f, 4) });
    }

    gui::cursor({ 0, gui::cursor().y + 1 }); // 1px padding

    // patterns
    for (int i = 0; i < pattern_page; ++i) {
        int r = g_pattern_scroll + i;

        gui::item_size({ CN, settings.row_height });
        gui::Box box = gui::item_box();

        sprintf(str, "%02X", r);
        dc.rgb(color::ROW_NUMBER);
        dc.text(box.pos + ivec2(6, text_offset), str);

        gui::item_size({ CC, settings.row_height });
        for (int c = 0; c < 3; ++c) {
            gui::same_line();
            gui::Box box = gui::item_box();
            gt::Pattern& patt = g_song.patterns[patt_nums[c]];
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
                dc.text(t, "\x0b\x0c\x0d");
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
    gui::cursor({ 0, gui::cursor().y + 1 });
    gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
    gui::separator();


    // scroll bars
    gui::cursor({ app::CANVAS_WIDTH - 80, cursor.y });
    gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
    {
        int page = g_song_page;
        int max_scroll = std::max(0, g_song.song_len - page);
        gui::item_size({ app::SCROLL_WIDTH, page * settings.row_height + 2 });
        if (gui::vertical_drag_bar(g_song_scroll, 0, max_scroll, page)) g_follow = false;
    }
    {
        gui::item_size({ app::SCROLL_WIDTH, app::BUTTON_HEIGHT });
        gui::vertical_drag_button(g_song_page, settings.row_height);
    }
    {
        int page = pattern_page;
        int max_scroll = std::max<int>(0, max_pattern_len - page);
        gui::item_size({ app::SCROLL_WIDTH, page * settings.row_height + 2});
        if (gui::vertical_drag_bar(g_pattern_scroll, 0, max_scroll, page)) g_follow = false;
    }
    gui::align(gui::Align::Center);


    int total_table_height = gui::cursor().y - cursor.y;
    gui::cursor({ 280 + app::SCROLL_WIDTH, cursor.y });
    gui::same_line();
    gui::item_size({ 75, total_table_height });
    gui::separator();
    gui::cursor(gui::cursor());
    gui::same_line(false);

    // buttons

    // order edit
    gui::item_size({ 55, app::BUTTON_HEIGHT });
    if (g_edit_mode == EditMode::Song && !(is_playing && g_follow)) {
        int& pos = g_cursor_song_row;
        int& len = g_song.song_len;
        assert(pos < len);

        gui::disabled(!(len < MAX_SONG_ROWS && pos <= len));
        if (gui::button(gui::Icon::AddRowAbove)) {
            for (auto& order : g_song.song_order) {
                std::rotate(order.begin() + pos, order.end() - 1, order.end());
                order[pos] = order[pos + 1];
            }
            if (g_song.song_loop >= pos) ++g_song.song_loop;
            ++len;
        }
        if (gui::button(gui::Icon::AddRowBelow)) {
            ++pos;
            for (auto& order : g_song.song_order) {
                std::rotate(order.begin() + pos, order.end() - 1, order.end());
                order[pos] = order[pos - 1];
            }
            if (g_song.song_loop >= pos) ++g_song.song_loop;
            ++len;
        }
        gui::disabled(!(len > 1 && pos < len));
        if (gui::button(gui::Icon::DeleteRow)) {
            for (auto& order : g_song.song_order) {
                order[pos] = {};
                std::rotate(order.begin() + pos, order.begin() + pos + 1, order.end());
            }
            --len;
            if (pos > 0 && g_song.song_loop > pos) --g_song.song_loop;
            if (g_song.song_loop >= len) g_song.song_loop = len - 1;
            if (pos >= len) --pos;
        }
        gui::disabled(g_song.song_loop == g_cursor_song_row);
        if (gui::button(gui::Icon::JumpBack)) {
            g_song.song_loop = g_cursor_song_row;
        }
        gui::disabled(false);

        if (gui::button(gui::Icon::EditRow)) {
            init_order_edit();
        }
    }


    // pattern edit
    gt::Pattern& patt = g_song.patterns[patt_nums[g_cursor_chan]];
    if (g_edit_mode == EditMode::Pattern && g_cursor_pattern_row < patt.len && !(is_playing && g_follow)) {
        auto& rows = patt.rows;
        int&  len  = patt.len;
        int&  pos  = g_cursor_pattern_row;

        gui::disabled(!(len < gt::MAX_PATTROWS));
        if (gui::button(gui::Icon::AddRowAbove)) {
            std::rotate(rows.begin() + pos, rows.end() - 1, rows.end());
            rows[pos] = {};
            ++len;
        }
        if (gui::button(gui::Icon::AddRowBelow)) {
            ++pos;
            ++len;
            std::rotate(rows.begin() + pos, rows.end() - 1, rows.end());
            rows[pos] = {};
        }
        gui::disabled(!(len > 1));
        if (gui::button(gui::Icon::DeleteRow)) {
            std::rotate(rows.begin() + pos, rows.begin() + pos + 1, rows.end());
            --len;
            if (pos >= len) --pos;
        }
        gui::disabled(false);


        gt::PatternRow& row  = patt.rows[pos];
        // note edit buttons
        {
            gui::separator();
            if (gui::button(gui::Icon::Record, g_recording)) {
                g_recording = !g_recording;
            }
            gui::disabled(row.note == gt::KEYOFF);
            if (gui::button("\x0b\x0c\x0d")) {
                row.note  = gt::KEYOFF;
                row.instr = 0;
            }
            gui::disabled(row.note == gt::REST);
            if (gui::button(gui::Icon::X)) {
                row.note  = gt::REST;
                row.instr = 0;
            }
            gui::disabled(false);
        }
        // command edit buttons
        {
            gui::separator();
            if (gui::button(gui::Icon::DotDotDot)) {
                g_dialog = Dialog::CommandEdit;
                command_edit::init(command_edit::Location::Pattern, row.command, row.data, [&row](uint8_t cmd, uint8_t data) {
                    g_dialog    = Dialog::None;
                    row.command = cmd;
                    row.data    = data;
                });
            }
            gui::disabled(row.command == 0);
            if (gui::button(gui::Icon::X)) {
                row.command = 0;
                row.data = 0;
            }

            static int copy_command      = -1;
            static int copy_command_data = 0;
            if (gui::button(gui::Icon::Copy)) {
                copy_command      = row.command;
                copy_command_data = row.data;
            }
            gui::disabled(copy_command == -1);
            if (gui::button(gui::Icon::Paste)) {
                row.command = copy_command;
                row.data    = copy_command_data;
            }
            gui::disabled(false);
        }


        // navigation
        gui::separator();
        gui::disabled(g_cursor_pattern_row == 0);
        if (gui::button(gui::Icon::MoveUp)) {
            --g_cursor_pattern_row;
            g_follow = false;
            g_pattern_scroll = clamp(g_pattern_scroll, g_cursor_pattern_row - pattern_page + 1, g_cursor_pattern_row);
        }
        gui::disabled(g_cursor_pattern_row >= patt.len - 1);
        if (gui::button(gui::Icon::MoveDown)) {
            ++g_cursor_pattern_row;
            g_follow = false;
            g_pattern_scroll = clamp(g_pattern_scroll, g_cursor_pattern_row - pattern_page + 1, g_cursor_pattern_row);
        }
        gui::disabled(false);
    }


    if (g_dialog == Dialog::OrderEdit) draw_order_edit();
    if (g_dialog == Dialog::CommandEdit) command_edit::draw();

    // piano
    if (piano::draw(&g_follow) && g_edit_mode == EditMode::Pattern && g_recording) {
        assert(g_cursor_pattern_row < patt.len);
        gt::PatternRow& row  = patt.rows[g_cursor_pattern_row];
        row.note  = piano::note() + gt::FIRSTNOTE;
        row.instr = piano::instrument();
    }
}

} // namespace song_view
