#include "song_view.hpp"

#include "app.hpp"
#include "command_edit.hpp"
#include "gtplayer.hpp"
#include "gui.hpp"
#include "instrument_view.hpp"
#include "log.hpp"
#include "piano.hpp"
#include "settings_view.hpp"
#include "sid.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstring>



namespace song_view {
namespace {

enum {
    CN = 28,
    CC = 84,
};

enum class EditMode {
    Song,
    SongMark,
    Pattern,
    PatternMark,
    Follow,
};


gt::Song&                      g_song = app::song();
int                            g_song_page;
bool                           g_recording;
EditMode                       g_edit_mode;
int                            g_song_scroll;
int                            g_pattern_scroll;
int                            g_cursor_pattern_row;
int                            g_cursor_song_row;
int                            g_cursor_chan;
int                            g_cursor_instr;
int                            g_transpose;
bool                           g_mark_edit;
int                            g_mark_chan;
int                            g_mark_row;
bool                           g_show_order_edit_window;
bool                           g_show_pattern_edit_window;
std::array<bool, gt::MAX_PATT> g_pattern_empty;
std::array<bool, gt::MAX_PATT> g_pattern_marked;


struct SongCopyBuffer {
    gt::Array2<gt::OrderRow, gt::MAX_CHN, gt::MAX_SONG_ROWS> order;
    int num_chans;
    int len;
};


void check_empty_patterns() {
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


void init_order_edit() {
    g_show_order_edit_window = true;
    g_transpose = g_song.song_order[g_cursor_chan][g_cursor_song_row].trans;
    check_empty_patterns();

    g_pattern_marked = {};
    if (g_edit_mode != EditMode::SongMark) {
        g_mark_row  = g_cursor_song_row;
        g_mark_chan = g_cursor_chan;
    }
    int mark_row_min  = std::min(g_mark_row, g_cursor_song_row);
    int mark_row_max  = std::max(g_mark_row, g_cursor_song_row);
    int mark_chan_min = std::min(g_mark_chan, g_cursor_chan);
    int mark_chan_max = std::max(g_mark_chan, g_cursor_chan);
    for (int c = mark_chan_min; c <= mark_chan_max; ++c) {
        for (int r = mark_row_min; r <= mark_row_max ; ++r) {
            auto row = g_song.song_order[c][r];
            g_pattern_marked[row.pattnum] = true;
        }
    }
}


void draw_order_edit() {
    if (!g_show_order_edit_window) return;
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
    gui::text("SET PATTERN");
    gui::separator();

    char str[32];
    gui::item_size({ CW, row_h });
    for (int i = 0; i < gt::MAX_PATT; ++i) {
        gui::same_line(i % 8 != 0);
        sprintf(str, "%02X", i);
        gui::button_style(g_pattern_empty[i] ? gui::ButtonStyle::Shaded : gui::ButtonStyle::Normal);
        if (gui::button(str, g_pattern_marked[i])) {
            int mark_row_min  = std::min(g_mark_row, g_cursor_song_row);
            int mark_row_max  = std::max(g_mark_row, g_cursor_song_row);
            int mark_chan_min = std::min(g_mark_chan, g_cursor_chan);
            int mark_chan_max = std::max(g_mark_chan, g_cursor_chan);
            for (int c = mark_chan_min; c <= mark_chan_max; ++c) {
                for (int r = mark_row_min; r <= mark_row_max ; ++r) {
                    g_song.song_order[c][r].pattnum = i;
                }
            }
            g_show_order_edit_window = false;
        }
    }
    gui::button_style(gui::ButtonStyle::Normal);
    gui::item_size({ WIDTH, app::BUTTON_HEIGHT });

    sprintf(str, "TRANSPOSE %c%X", "+-"[g_transpose < 0], abs(g_transpose));
    if (gui::slider(WIDTH, str, g_transpose, -0xf, 0xe)) {
        int mark_row_min  = std::min(g_mark_row, g_cursor_song_row);
        int mark_row_max  = std::max(g_mark_row, g_cursor_song_row);
        int mark_chan_min = std::min(g_mark_chan, g_cursor_chan);
        int mark_chan_max = std::max(g_mark_chan, g_cursor_chan);
        for (int c = mark_chan_min; c <= mark_chan_max; ++c) {
            for (int r = mark_row_min; r <= mark_row_max ; ++r) {
                g_song.song_order[c][r].trans = g_transpose;;
            }
        }
    }

    gui::item_size({ WIDTH, app::BUTTON_HEIGHT });
    gui::separator();
    if (gui::button("CLOSE")) g_show_order_edit_window = false;
    gui::end_window();
}


void update_mark(int& row, int page, int len, int& scroll) {
    constexpr float SCROLL_DELAY = 1.0f;
    if (!g_mark_edit) return;
    static float mark_scroll = 0;
    if (gui::touch::just_released()) {
        g_mark_edit = false;
        mark_scroll = 0;
    }
    // auto scroll
    int dy = gui::touch::pos().y - gui::cursor().y;
    int v = std::min(dy, 0) + std::max(0, dy - settings_view::settings().row_height * page);
    mark_scroll += v * gui::frame_time();
    while (mark_scroll < -SCROLL_DELAY) {
        mark_scroll += SCROLL_DELAY;
        scroll = std::max(0, scroll - 1);
    }
    while (mark_scroll > SCROLL_DELAY) {
        mark_scroll -= SCROLL_DELAY;
        scroll = std::min(len - page, scroll + 1);
    }
    row = dy / settings_view::settings().row_height;
    row -= dy < 0;
    row += scroll;
    row = clamp(row, 0, len - 1);
    g_cursor_chan = (gui::touch::pos().x - CN) / CC;
    g_cursor_chan = clamp(g_cursor_chan, 0, 2);
}


} // namespace


int channel() { return g_cursor_chan; }

int song_position() {
    return std::min(g_cursor_song_row, g_song.song_len - 1);
}

int cursor_instrument() {
    return g_cursor_instr;
}

void reset() {
    g_song_page                = 8;
    g_recording                = false;
    g_edit_mode                = EditMode::Pattern;
    g_song_scroll              = 0;
    g_pattern_scroll           = 0;
    g_cursor_pattern_row       = 0;
    g_cursor_song_row          = 0;
    g_cursor_chan              = 0;
    g_transpose                = 0;
    g_mark_edit                = false;
    g_show_order_edit_window   = false;
    g_show_pattern_edit_window = false;
}

bool get_follow() {
    return g_edit_mode == EditMode::Follow;
}
void toggle_follow() {
    if (g_edit_mode == EditMode::Follow) g_edit_mode = EditMode::Pattern;
    else g_edit_mode = EditMode::Follow;
}


void draw_pattern() {
//     int patt_num = g_song.song_order[g_cursor_chan][g_cursor_song_row].pattnum;
//     gt::Pattern& patt = g_song.patterns[patt_num];
//     // TODO
}


void draw() {
    g_cursor_instr = 0;

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

    ivec2 cursor = gui::cursor();
    gui::cursor({ 0, cursor.y + 1 }); // 1px padding

    int pattern_page;
    {
        int height_left = app::canvas_height() - cursor.y - piano::HEIGHT - app::BUTTON_HEIGHT;
        int total_rows = (height_left - 4) / settings.row_height;
        g_song_page = clamp(g_song_page, 0, total_rows);
        pattern_page = total_rows - g_song_page;
    }

    std::array<int, 3> patt_nums;
    if (g_edit_mode == EditMode::Follow) {
        // auto scroll
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

    if (g_edit_mode == EditMode::SongMark) {
        update_mark(g_cursor_song_row, g_song_page, g_song.song_len, g_song_scroll);
    }

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
            if (r >= int(g_song.song_len)) continue;
            gt::OrderRow& row = g_song.song_order[c][r];

            gui::ButtonState state = gui::button_state(box);
            if (state == gui::ButtonState::Released) {
                g_edit_mode       = EditMode::Song;
                g_cursor_chan     = c;
                g_cursor_song_row = r;
                for (int k = 0; k < gt::MAX_CHN; ++k) {
                    patt_nums[k] = g_song.song_order[k][r].pattnum;
                }
            }
            if (gui::hold()) {
                gui::set_active_item(&row); // disable hovering on other items
                g_edit_mode       = EditMode::SongMark;
                g_mark_edit       = true;
                g_cursor_chan     = c;
                g_cursor_song_row = r;
                g_mark_chan       = c;
                g_mark_row        = r;
            }

            int mark_row_min  = std::min(g_mark_row, g_cursor_song_row);
            int mark_row_max  = std::max(g_mark_row, g_cursor_song_row);
            int mark_chan_min = std::min(g_mark_chan, g_cursor_chan);
            int mark_chan_max = std::max(g_mark_chan, g_cursor_chan);

            dc.rgb(color::BACKGROUND_ROW);
            if (r == g_cursor_song_row) dc.rgb(color::HIGHLIGHT_ROW);
            if (r == player_song_rows[c]) dc.rgb(color::PLAYER_ROW);
            bool is_marked = g_edit_mode == EditMode::SongMark && mark_chan_min <= c && c <= mark_chan_max && mark_row_min <= r && r <= mark_row_max;
            if (is_marked) dc.rgb(color::MARKED_ROW);


            box.pos.x += 1;
            box.size.x -= 2;
            dc.fill(box);


            if (g_edit_mode == EditMode::SongMark) {
                if (is_marked) {
                    dc.rgb(color::BUTTON_ACTIVE);
                    dc.fill({ box.pos, ivec2(1, box.size.y) });
                    dc.fill({ box.pos + ivec2(box.size.x - 1, 0), ivec2(1, box.size.y) });
                    if (r == mark_row_min) {
                        dc.fill({ box.pos, ivec2(box.size.x, 1) });
                    }
                    if (r == mark_row_max) {
                        dc.fill({ box.pos + ivec2(0, box.size.y - 1), ivec2(box.size.x, 1) });
                    }
                }
            }
            else if (g_edit_mode == EditMode::Song && c == g_cursor_chan && r == g_cursor_song_row) {
                dc.rgb(color::BUTTON_ACTIVE);
                dc.box(box, gui::BoxStyle::Cursor);
            }

            if (state != gui::ButtonState::Normal) {
                dc.rgb(color::BUTTON_PRESSED);
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
            dc.rgb(color::WHITE);
            dc.text(box.pos + ivec2(CN + CC * 3 - 9, text_offset), "\x05");
        }
    }

    gui::cursor({ 0, gui::cursor().y + 1 }); // 1px padding

    // pattern bar
    gui::item_size({ CN, settings.row_height });
    gui::item_box();
    gui::item_size({ CC, app::BUTTON_HEIGHT });
    auto levels = app::sid().get_env_levels();
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
        // dc.rgb(color::mix(color::GREEN, 0, 0.2f));
        dc.rgb(color::mix(color::C64[11], 0, 0.2f));
        dc.fill({ p + ivec2(29, 13), ivec2(levels[c] * 46.0f + 0.9f, 4) });
    }

    gui::cursor({ 0, gui::cursor().y + 1 }); // 1px padding

    // pattern marking
    if (g_edit_mode == EditMode::PatternMark) {
        update_mark(g_cursor_pattern_row, pattern_page, max_pattern_len, g_pattern_scroll);
    }

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
            gt::PatternRow& row = patt.rows[r];

            gui::ButtonState state = gui::button_state(box);
            if (state == gui::ButtonState::Released) {
                g_edit_mode          = EditMode::Pattern;
                g_cursor_chan        = c;
                g_cursor_pattern_row = r;
            }
            if (gui::hold()) {
                gui::set_active_item(&row); // disable hovering on other items
                g_edit_mode          = EditMode::PatternMark;
                g_mark_edit          = true;
                g_cursor_chan        = c;
                g_cursor_pattern_row = r;
                g_mark_chan          = c;
                g_mark_row           = r;
            }

            if (g_edit_mode == EditMode::Pattern && c == g_cursor_chan && r == g_cursor_pattern_row) {
                g_cursor_instr = row.instr;
            }

            int mark_row_min  = std::min(g_mark_row, g_cursor_pattern_row);
            int mark_row_max  = std::max(g_mark_row, g_cursor_pattern_row);
            int mark_chan_min = std::min(g_mark_chan, g_cursor_chan);
            int mark_chan_max = std::max(g_mark_chan, g_cursor_chan);

            dc.rgb(color::BACKGROUND_ROW);
            if (r % settings.row_highlight == 0) {
                dc.rgb(color::HIGHLIGHT_ROW);
            }
            if (patt_nums[c] == player_patt_nums[c] && r == player_patt_rows[c]) {
                dc.rgb(color::PLAYER_ROW);
            }
            bool is_marked = g_edit_mode == EditMode::PatternMark && mark_chan_min <= c && c <= mark_chan_max && mark_row_min <= r && r <= mark_row_max;
            if (is_marked) {
                dc.rgb(color::MARKED_ROW);
            }

            box.pos.x += 1;
            box.size.x -= 2;
            dc.fill(box);

            if (g_edit_mode == EditMode::PatternMark) {
                if (is_marked) {
                    dc.rgb(color::BUTTON_ACTIVE);
                    dc.fill({ box.pos, ivec2(1, box.size.y) });
                    dc.fill({ box.pos + ivec2(box.size.x - 1, 0), ivec2(1, box.size.y) });
                    if (r == mark_row_min) {
                        dc.fill({ box.pos, ivec2(box.size.x, 1) });
                    }
                    if (r == mark_row_max || r == patt.len - 1) {
                        dc.fill({ box.pos + ivec2(0, box.size.y - 1), ivec2(box.size.x, 1) });
                    }
                }
            }
            else if ((g_edit_mode == EditMode::Pattern || g_edit_mode == EditMode::Follow) && c == g_cursor_chan && r == g_cursor_pattern_row) {
                dc.rgb(color::BUTTON_ACTIVE);
                dc.box(box, gui::BoxStyle::Cursor);
            }

            if (state != gui::ButtonState::Normal) {
                dc.rgb(color::BUTTON_PRESSED);
                dc.box(box, gui::BoxStyle::Cursor);
            }

            ivec2 t = box.pos + ivec2(5, text_offset);
            dc.rgb(color::WHITE);
            if (row.note == gt::REST) {
                dc.rgb(color::DARK_GREY);
                dc.text(t, "\x01\x01\x01");
            }
            else if (row.note == gt::KEYOFF) {
                dc.text(t, "\x0a\x0b\x0c");
            }
            else if (row.note == gt::KEYON) {
                dc.text(t, "\x0d\x0e\x0f");
            }
            else {
                sprintf(str, "%c%c%d", "CCDDEFFGGAAB"[row.note % 12],
                             // "-#-#--#-#-#-"
                             "\x12\x13\x12\x13\x12\x12\x13\x12\x13\x12\x13\x12"[row.note % 12],
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
                uint8_t data = row.data;
                if (data > 0 && row.command == gt::CMD_VIBRATO)   data -= gt::STBL_VIB_START;
                if (data > 0 && row.command == gt::CMD_FUNKTEMPO) data -= gt::STBL_FUNK_START;
                sprintf(str, "%X%02X", row.command, data);
                dc.text(t, str);
            }
        }
    }
    gui::cursor({ 0, gui::cursor().y + 1 });
    gui::item_size(app::CANVAS_WIDTH);
    gui::separator();


    // scroll bars
    gui::cursor({ app::CANVAS_WIDTH - 80, cursor.y });
    gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
    {
        int max_scroll = std::max(0, g_song.song_len - g_song_page);
        gui::item_size({ app::SCROLL_WIDTH, g_song_page * settings.row_height + 2 });
        if (gui::vertical_drag_bar(g_song_scroll, 0, max_scroll, g_song_page)) {
            if (g_edit_mode == EditMode::Follow) g_edit_mode = EditMode::Pattern;
        }
    }
    {
        gui::item_size({ app::SCROLL_WIDTH, app::BUTTON_HEIGHT });
        gui::vertical_drag_button(g_song_page, settings.row_height);
    }
    {
        int max_scroll = std::max<int>(0, max_pattern_len - pattern_page);
        gui::item_size({ app::SCROLL_WIDTH, pattern_page * settings.row_height + 2});
        if (gui::vertical_drag_bar(g_pattern_scroll, 0, max_scroll, pattern_page)) {
            if (g_edit_mode == EditMode::Follow) g_edit_mode = EditMode::Pattern;
        }
    }
    gui::align(gui::Align::Center);


    int total_table_height = gui::cursor().y - cursor.y;
    gui::cursor({ 280 + app::SCROLL_WIDTH, cursor.y });
    gui::same_line();
    gui::item_size(total_table_height);
    gui::separator();
    gui::cursor(gui::cursor());
    gui::same_line(false);

    // buttons

    // order edit
    static SongCopyBuffer song_copy_buffer;

    gui::item_size({ 55, app::BUTTON_HEIGHT });
    if (g_edit_mode == EditMode::Song) {
        int& pos = g_cursor_song_row;
        int& len = g_song.song_len;
        assert(pos < len);

        if (gui::button(gui::Icon::Paste)) {
            auto const& b = song_copy_buffer;
            for (int c = 0; c < b.num_chans; ++c) {
                if (g_cursor_chan + c >= 3) break;
                for (int i = 0; i < b.len; ++i) {
                    if (g_cursor_song_row + i >= g_song.song_len) break;
                    g_song.song_order[g_cursor_chan + c][g_cursor_song_row + i] = b.order[c][i];
                }
            }
        }

        gui::disabled(!(len < gt::MAX_SONG_ROWS && pos <= len));
        if (gui::button(gui::Icon::AddRowAbove)) {
            for (auto& order : g_song.song_order) {
                std::rotate(order.begin() + pos, order.end() - 1, order.end());
                order[pos] = order[pos + 1]; // copy row
            }
            if (g_song.song_loop >= pos) ++g_song.song_loop;
            ++len;
        }
        if (gui::button(gui::Icon::AddRowBelow)) {
            ++pos;
            for (auto& order : g_song.song_order) {
                std::rotate(order.begin() + pos, order.end() - 1, order.end());
                order[pos] = order[pos - 1]; // copy row
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

        if (gui::button(gui::Icon::Edit)) {
            init_order_edit();
        }
        gui::separator();
    }

    else if (g_edit_mode == EditMode::SongMark) {
        int mark_row_min  = std::min(g_mark_row, g_cursor_song_row);
        int mark_row_max  = std::max(g_mark_row, g_cursor_song_row);
        int mark_chan_min = std::min(g_mark_chan, g_cursor_chan);
        int mark_chan_max = std::max(g_mark_chan, g_cursor_chan);
        if (gui::button(gui::Icon::Copy)) {
            auto& b = song_copy_buffer;
            b.num_chans = mark_chan_max - mark_chan_min + 1;
            b.len       = mark_row_max - mark_row_min + 1;
            for (int c = 0; c < b.num_chans; ++c) {
                for (int i = 0; i < b.len; ++i) {
                    b.order[c][i] = g_song.song_order[mark_chan_min + c][mark_row_min + i];
                }
            }
        }
        if (gui::button(gui::Icon::Edit)) {
            init_order_edit();
        }

        gui::separator();
    }


    // pattern edit
    static std::array<gt::Pattern, 3> pattern_copy_buffer = {};
    static uint8_t                    pattern_copy_flags  = 0;

    gt::Pattern& patt = g_song.patterns[patt_nums[g_cursor_chan]];
    if (g_edit_mode == EditMode::Pattern && g_cursor_pattern_row < patt.len) {

        if (gui::button(gui::Icon::Paste)) {
            for (int c = 0; c < 3; ++c) {
                if (g_cursor_chan + c >= 3) break;
                gt::Pattern const& src = pattern_copy_buffer[c];
                gt::Pattern&       dst = g_song.patterns[patt_nums[g_cursor_chan + c]];

                for (int i = 0; i < src.len; ++i) {
                    if (g_cursor_pattern_row + i >= dst.len) break;
                    if (pattern_copy_flags & 1) {
                        dst.rows[g_cursor_pattern_row + i].note  = src.rows[i].note;
                        dst.rows[g_cursor_pattern_row + i].instr = src.rows[i].instr;
                    }
                    if (pattern_copy_flags & 2) {
                        dst.rows[g_cursor_pattern_row + i].command = src.rows[i].command;
                        dst.rows[g_cursor_pattern_row + i].data    = src.rows[i].data;
                    }
                }
            }
        }
        if (gui::button(gui::Icon::ChangeLength)) {
            g_show_pattern_edit_window ^= 1;
        }
        if (g_show_pattern_edit_window) {
            gui::Box box = gui::begin_window({ app::CANVAS_WIDTH - 12, app::BUTTON_HEIGHT * 5 + gui::FRAME_WIDTH * 2 });
            gui::item_size({ box.size.x, app::BUTTON_HEIGHT });
            gui::text("PATTERN %02X", patt_nums[g_cursor_chan]);
            gui::separator();
            gui::slider(box.size.x, "LENGTH %02X", patt.len, 1, gt::MAX_PATTROWS);
            g_cursor_pattern_row = std::min(g_cursor_pattern_row, patt.len - 1);
            gui::item_size({ box.size.x, app::BUTTON_HEIGHT });
            if (gui::button("RESIZE EMPTY PATTERNS")) {
                check_empty_patterns();
                for (int i = 0; i < gt::MAX_PATT; ++i) {
                    if (!g_pattern_empty[i]) continue;
                    g_song.patterns[i].len = patt.len;
                }
            }
            gui::item_size({ box.size.x / 2, app::BUTTON_HEIGHT });
            gui::disabled(patt.len <= 1);
            if (gui::button("SHRINK")) {
                gt::Pattern p;
                p.len = patt.len / 2;
                for (int i = 0; i < p.len; ++i) {
                    p.rows[i] = patt.rows[i * 2];
                }
                patt = p;
            }
            gui::same_line();
            gui::disabled(patt.len > gt::MAX_PATTROWS / 2);
            if (gui::button("EXPAND")) {
                gt::Pattern p;
                p.len = patt.len * 2;
                for (int i = 0; i < patt.len; ++i) {
                    p.rows[i * 2] = patt.rows[i];
                }
                patt = p;
            }
            gui::disabled(false);
            gui::item_size({ box.size.x, app::BUTTON_HEIGHT });
            gui::separator();
            if (gui::button("CLOSE")) {
                g_show_pattern_edit_window = false;
                for (int i = patt.len; i < gt::MAX_PATTROWS; ++i) {
                    patt.rows[i] = {};
                }
            }
            gui::end_window();
            gui::item_size({ 55, app::BUTTON_HEIGHT });
        }

        gui::separator();


        gt::PatternRow& row  = patt.rows[g_cursor_pattern_row];
        // note edit buttons
        if (gui::button(gui::Icon::Record, g_recording)) {
            g_recording = !g_recording;
        }
        if (gui::button(row.note != gt::KEYOFF ? "\x0a\x0b\x0c" : "\x0d\x0e\x0f")) {
            row.note  = row.note != gt::KEYOFF ? gt::KEYOFF : gt::KEYON;
            row.instr = 0;
        }
        gui::disabled(row.note == gt::REST);
        if (gui::button(gui::Icon::X)) {
            row.note  = gt::REST;
            row.instr = 0;
        }
        gui::disabled(false);
        gui::separator();
        // command edit buttons
        if (gui::button(gui::Icon::Edit)) {
            command_edit::init(command_edit::Location::Pattern, row.command, row.data, [&row](uint8_t cmd, uint8_t data) {
                row.command = cmd;
                row.data    = data;
            });
        }
        // long press goes to table if table pointer command is selected
        if (row.command >= gt::CMD_SETWAVEPTR && row.command <= gt::CMD_SETFILTERPTR && gui::hold()) {
            gui::set_active_item((void*)1);
            app::go_to_instrument_view();
            piano::set_instrument(row.data);
            instrument_view::select_table(row.command - gt::CMD_SETWAVEPTR);
        }

        gui::disabled(row.command == 0);
        if (gui::button(gui::Icon::X)) {
            row.command = 0;
            row.data = 0;
        }
        gui::disabled(false);
        gui::separator();


        // play from cursor
        gui::item_size({ 55, total_table_height - 260 });
        gui::item_box();
        gui::separator();
        gui::item_size({ 55, app::BUTTON_HEIGHT });
        if (gui::button(gui::Icon::Play)) {
            app::player().m_start_song_pos.fill(g_cursor_song_row);
            app::player().m_start_patt_pos.fill(g_cursor_pattern_row);
            app::player().set_action(gt::Player::Action::Start);
        }

    }

    if (g_edit_mode == EditMode::PatternMark) {
        int mark_row_min  = std::min(g_mark_row, g_cursor_pattern_row);
        int mark_row_max  = std::max(g_mark_row, g_cursor_pattern_row);
        int mark_chan_min = std::min(g_mark_chan, g_cursor_chan);
        int mark_chan_max = std::max(g_mark_chan, g_cursor_chan);
        auto copy_to_pattern_buffer = [&] {
            int num_chans = mark_chan_max - mark_chan_min + 1;
            for (int c = 0; c < 3; ++c) {
                gt::Pattern& dst = pattern_copy_buffer[c];
                if (c >= num_chans) {
                    dst.len = 0;
                    continue;
                }
                gt::Pattern const& src = g_song.patterns[patt_nums[mark_chan_min + c]];
                int max = std::min(mark_row_max, src.len + 1);
                dst.len = std::max(0, max - mark_row_min + 1);
                for (int i = 0; i < dst.len; ++i) {
                    dst.rows[i] = src.rows[i + mark_row_min];
                }
            }
        };

        // copy all
        if (gui::button(gui::Icon::Copy)) {
            copy_to_pattern_buffer();
            pattern_copy_flags = 3;
        }
        // delete
        if (gui::button(gui::Icon::X)) {
            for (int c = mark_chan_min; c <= mark_chan_max; ++c) {
                gt::Pattern& patt = g_song.patterns[patt_nums[c]];
                for (int i = mark_row_min; i <= mark_row_max; ++i) {
                    if (i >= patt.len) break;
                    patt.rows[i] = gt::PatternRow{};
                }
            }
        }
        gui::separator();

        // transpose up
        if (gui::button("\x09\x10")) {
            for (int c = mark_chan_min; c <= mark_chan_max; ++c) {
                gt::Pattern& patt = g_song.patterns[patt_nums[c]];
                for (int i = mark_row_min; i <= mark_row_max; ++i) {
                    if (i >= patt.len) break;
                    uint8_t& note = patt.rows[i].note;
                    if (note >= gt::FIRSTNOTE && note < gt::LASTNOTE) ++note;
                }
            }
        }
        // transpose down
        if (gui::button("\x09\x11")) {
            for (int c = mark_chan_min; c <= mark_chan_max; ++c) {
                gt::Pattern& patt = g_song.patterns[patt_nums[c]];
                for (int i = mark_row_min; i <= mark_row_max; ++i) {
                    if (i >= patt.len) break;
                    uint8_t& note = patt.rows[i].note;
                    if (note > gt::FIRSTNOTE && note <= gt::LASTNOTE) --note;
                }
            }
        }

        // set instrument
        // TODO: better icon
        if (gui::button(gui::Icon::Piano)) {
            for (int c = mark_chan_min; c <= mark_chan_max; ++c) {
                gt::Pattern& patt = g_song.patterns[patt_nums[c]];
                for (int i = mark_row_min; i <= mark_row_max; ++i) {
                    if (i >= patt.len) break;
                    gt::PatternRow& row = patt.rows[i];
                    if (row.instr > 0) row.instr = piano::instrument();
                }
            }
        }

        // copy notes
        if (gui::button(gui::Icon::Copy)) {
            copy_to_pattern_buffer();
            pattern_copy_flags = 1;
        }
        // delete notes
        if (gui::button(gui::Icon::X)) {
            for (int c = mark_chan_min; c <= mark_chan_max; ++c) {
                gt::Pattern& patt = g_song.patterns[patt_nums[c]];
                for (int i = mark_row_min; i <= mark_row_max; ++i) {
                    if (i >= patt.len) break;
                    patt.rows[i].note  = gt::REST;
                    patt.rows[i].instr = 0;
                }
            }
        }
        gui::separator();

        // edit command
        if (gui::button(gui::Icon::Edit)) {
            gt::Pattern const&    patt = g_song.patterns[patt_nums[mark_chan_min]];
            gt::PatternRow const& row  = patt.rows[mark_row_min];
            command_edit::init(command_edit::Location::Pattern, row.command, row.data, [&](uint8_t cmd, uint8_t data) {
                for (int c = mark_chan_min; c <= mark_chan_max; ++c) {
                    gt::Pattern& patt = g_song.patterns[patt_nums[c]];
                    for (int i = mark_row_min; i <= mark_row_max; ++i) {
                        if (i >= patt.len) break;
                        patt.rows[i].command = cmd;
                        patt.rows[i].data    = data;
                    }
                }
            });
        }
        // copy command
        if (gui::button(gui::Icon::Copy)) {
            copy_to_pattern_buffer();
            pattern_copy_flags = 2;
        }
        // delete command
        if (gui::button(gui::Icon::X)) {
            for (int c = mark_chan_min; c <= mark_chan_max; ++c) {
                gt::Pattern& patt = g_song.patterns[patt_nums[c]];
                for (int i = mark_row_min; i <= mark_row_max; ++i) {
                    if (i >= patt.len) break;
                    patt.rows[i].command = 0;
                    patt.rows[i].data    = 0;
                }
            }
        }
        gui::separator();
    }


    draw_order_edit();
    command_edit::draw();
    piano::draw();
    if (g_edit_mode == EditMode::Pattern && g_recording && piano::note_on()) {
        assert(g_cursor_pattern_row < patt.len);
        gt::PatternRow& row  = patt.rows[g_cursor_pattern_row];
        row.note  = piano::note() + gt::FIRSTNOTE;
        row.instr = piano::instrument();
    }
}

} // namespace song_view
