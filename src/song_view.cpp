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
uint8_t         g_command            = 0;
uint8_t         g_command_data[16]   = {
    0,
    0, 0, 0,
    0,
    0, 0,
    0, 0, 0, 0, 0, 0,
    0x0f, // master volume
    0,
    0x06, // tempo
};
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
    if (g_dialog != Dialog::OrderEdit) return;

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


void draw_command_edit() {
    if (g_dialog != Dialog::CommandEdit) return;

    enum {
        PAGE = 16,
        TABLE_HEIGHT = PAGE * app::MAX_ROW_HEIGHT,
        WIDTH = app::CANVAS_WIDTH - 12,
        HEIGHT = app::BUTTON_HEIGHT * 5 + TABLE_HEIGHT + gui::FRAME_WIDTH * 3,
        C1 = 15 * 8 + 12 + 3 * 8 + 12 - 1,
        CC = 12 + 8 * 18,
    };

    gui::DrawContext& dc = gui::draw_context();
    char str[32];

    gui::begin_window({ WIDTH, HEIGHT });
    gui::item_size({ WIDTH, app::BUTTON_HEIGHT });
    gui::text("COMMAND EDIT");
    gui::separator();
    ivec2 cmd_cursor = gui::cursor();
    gui::item_size({ C1, TABLE_HEIGHT });
    gui::item_box();
    gui::same_line();
    gui::separator();
    ivec2 table_cursor = gui::cursor();
    gui::same_line(false);
    gui::item_size({ WIDTH, app::BUTTON_HEIGHT });
    gui::separator();
    ivec2 button_cursor = gui::cursor();
    gui::item_size({ WIDTH, app::BUTTON_HEIGHT });
    gui::item_box();
    gui::item_box();
    gui::item_box();
    gui::separator();
    if (gui::button("CLOSE")) {
        g_dialog = Dialog::None;
        g_command_row->command = g_command;
        g_command_row->data    = g_command_data[g_command];
    }


    gui::cursor(cmd_cursor);
    gui::align(gui::Align::Left);
    for (int i = 0; i < 16; ++i) {
        constexpr char const* CMD_LABELS[] = {
            "DO NOTHING",
            "PORTAMENTO UP",
            "PORTAMENTO DOWN",
            "TONE PORTAMENTO",
            "VIBRATO",
            "ATTACK/DECAY",
            "SUSTAIN/RELEASE",
            "WAVE",
            "WAVE TABLE",
            "PULSE TABLE",
            "FILTER TABLE",
            "FILTER CONTROL",
            "FILTER CUTOFF",
            "MASTER VOLUME",
            "FUNK TEMPO",
            "TEMPO",
        };
        ivec2 c = gui::cursor();
        gui::item_size({ C1, app::MAX_ROW_HEIGHT });
        if (gui::button("", i == g_command)) {
            g_command = i;
        }
        dc.rgb(color::WHITE);
        dc.text(c + ivec2(12 + 8 * 3 + 6 - 1, 6), CMD_LABELS[i]);

        gui::Box b = { c + ivec2(1, 1), ivec2(36 - 1, app::MAX_ROW_HEIGHT - 2) };
        dc.rgb(color::BACKGROUND_ROW);
        dc.fill(b);
        sprintf(str, "%X%02X", i, g_command_data[i]);
        gui::disabled(g_command != i);
        dc.rgb(color::CMDS[i]);
        dc.text(b.pos + 5, str);
        gui::disabled(false);
    }
    gui::align(gui::Align::Center);
    gui::cursor(button_cursor);

    auto& data = g_command_data[g_command];

    // handle all command that reference the speed table
    if ((g_command >= gt::CMD_PORTAUP && g_command <= gt::CMD_TONEPORTA) ||
        g_command == gt::CMD_VIBRATO ||
        g_command == gt::CMD_FUNKTEMPO)
    {
        int cmd_type = 0;
        if (g_command == gt::CMD_VIBRATO) cmd_type = 1;
        if (g_command == gt::CMD_FUNKTEMPO) cmd_type = 2;
        static int scrolls[3] = {};
        int& scroll = scrolls[cmd_type];

        gui::cursor(table_cursor);

        auto& ltable = g_song.ltable[gt::STBL];
        auto& rtable = g_song.rtable[gt::STBL];

        for (int i = 0; i < PAGE; ++i) {
            int r = i + scroll;
            if (r > 0) {
                if (g_command == gt::CMD_VIBRATO)   r += 0x20;
                if (g_command == gt::CMD_FUNKTEMPO) r += 0x40;
            }

            gui::item_size({ CC, app::MAX_ROW_HEIGHT });
            ivec2 p = gui::cursor() + ivec2(6);

            uint8_t lval = r > 0 ? ltable[r - 1] : 0;
            uint8_t rval = r > 0 ? rtable[r - 1] : 0;

            bool is_set = lval | rval;
            gui::button_style(is_set || r == 0 ? gui::ButtonStyle::Normal : gui::ButtonStyle::Shaded);
            if (gui::button("", data == r)) {
                data = r;
            }
            sprintf(str, "%02X", r);
            dc.text(p, str);
            p.x += 8 * 3;
            if (r == 0) {
                if (g_command == gt::CMD_TONEPORTA) dc.text(p, "TIE NOTE");
                else if (g_command == gt::CMD_FUNKTEMPO) dc.text(p, "NO CHANGE");
                else dc.text(p, "OFF");
            }
            if (is_set) {
                sprintf(str, "%02X\x80%02X", lval, rval);
                dc.text(p, str);
            }
        }
        gui::button_style(gui::ButtonStyle::Normal);

        gui::cursor(table_cursor + ivec2(CC, 0));
        gui::item_size({ app::SCROLL_WIDTH, TABLE_HEIGHT });

        gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
        int table_len = 32;
        int max_scroll = std::max(0, table_len - PAGE);
        gui::vertical_drag_bar(scroll, 0, max_scroll, PAGE);

        gui::cursor(button_cursor);
        if (data > 0) {
            uint8_t& lval = ltable[data - 1];
            uint8_t& rval = rtable[data - 1];

            if (g_command <= gt::CMD_TONEPORTA) {
                gui::item_size({ WIDTH, app::BUTTON_HEIGHT });
                if (gui::button("CALCULATE SPEED IN REALTIME", lval & 0x80)) {
                    if (lval & 0x80) {
                        uint16_t speed = 0;
                        if (g_command_row->note >= gt::FIRSTNOTE && g_command_row->note <= gt::LASTNOTE) {
                            int note = g_command_row->note - gt::FIRSTNOTE;
                            speed = gt::get_freq(note + 1) - gt::get_freq(note);
                            speed >>= rval;
                        }
                        lval = speed >> 8;
                        rval = speed & 0xff;
                    }
                    else {
                        lval = 0x80;
                        rval = 0;
                    }
                }
                if (lval & 0x80) {
                    gui::slider(WIDTH, "SHIFT %X", rval, 0, 8);
                }
                else {
                    gui::slider(WIDTH, "HI %02X", lval, 0, 0x7f);
                    gui::slider(WIDTH, "LO %02X", rval, 0, 0xff);
                }
            }
            else if (g_command == gt::CMD_VIBRATO) {
                gui::item_size({ WIDTH, app::BUTTON_HEIGHT });
                if (gui::button("CALCULATE SPEED IN REALTIME", lval & 0x80)) {
                    if (lval & 0x80 && g_command_row->note >= gt::FIRSTNOTE && g_command_row->note <= gt::LASTNOTE) {
                        int note = g_command_row->note - gt::FIRSTNOTE;
                        uint16_t speed = gt::get_freq(note + 1) - gt::get_freq(note);
                        rval = std::min(255, speed >> rval);
                    }
                    else {
                        rval = 0;
                    }
                    lval ^= 0x80;
                }
                int v = lval & 0x7f;
                gui::slider(WIDTH, "STEPS %02X", v, 0, 0x7f);
                lval = (lval & 0x80) | v;
                if (lval & 0x80) {
                    gui::slider(WIDTH, "SHIFT  %X", rval, 0, 8);
                }
                else {
                    gui::slider(WIDTH, "SPEED %02X", rval, 0, 0xff);
                }
            }
            else if (g_command == gt::CMD_FUNKTEMPO) {
                gui::slider(WIDTH, "EVEN %02X", lval, 0, 0xff);
                gui::slider(WIDTH, "ODD  %02X", rval, 0, 0xff);
            }
        }
    }
    else if (g_command == gt::CMD_SETAD) {
        int a = data >> 4;
        int d = data & 0xf;
        gui::slider(WIDTH, "ATTACK  %X", a, 0, 15);
        gui::slider(WIDTH, "DECAY   %X", d, 0, 15);
        data = (a << 4) | d;
    }
    else if (g_command == gt::CMD_SETSR) {
        int s = data >> 4;
        int r = data & 0xf;
        gui::slider(WIDTH, "SUSTAIN %X", s, 0, 15);
        gui::slider(WIDTH, "RELEASE %X", r, 0, 15);
        data = (s << 4) | r;
    }
    else if (g_command == gt::CMD_SETWAVE) {
        uint8_t v = data;
        for (int i = 0; i < 8; ++i) {
            gui::item_size({ WIDTH / 8 + i%2, app::BUTTON_HEIGHT });
            int mask = 0x80 >> i;
            int icon = int(gui::Icon::Noise) + i;
            if (gui::button(gui::Icon(icon), v & mask)) {
                v ^= mask;
            }
            gui::same_line();
        }
        gui::same_line(false);
        data = v;
    }
    else if (g_command >= gt::CMD_SETWAVEPTR && g_command <= gt::CMD_SETFILTERPTR) {
        gui::cursor(table_cursor);
        static int scroll = 0;

        gui::align(gui::Align::Left);
        gui::item_size({ CC, app::MAX_ROW_HEIGHT });
        for (int i = 0; i < PAGE; ++i) {
            int r = i + scroll;
            gt::Instrument const& instr = g_song.instruments[r];
            sprintf(str, "%02X %s", r, instr.name.data());
            gui::disabled(instr.ptr[g_command - gt::CMD_SETWAVEPTR] == 0);
            if (gui::button(str, data == r)) data = r;
        }
        gui::disabled(false);
        gui::align(gui::Align::Center);

        gui::cursor(table_cursor + ivec2(CC, 0));
        gui::item_size({ app::SCROLL_WIDTH, TABLE_HEIGHT });
        gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
        gui::vertical_drag_bar(scroll, 0, gt::MAX_INSTR - PAGE, PAGE);
    }
    else if (g_command == gt::CMD_SETFILTERCTRL) {
        gui::item_size({ WIDTH / 3, app::BUTTON_HEIGHT });
        if (gui::button("VOICE 1", data & 0x1)) data ^= 0x1;
        gui::same_line();
        if (gui::button("VOICE 2", data & 0x2)) data ^= 0x2;
        gui::same_line();
        if (gui::button("VOICE 3", data & 0x4)) data ^= 0x4;
        int v = data >> 4;
        gui::slider(WIDTH, "RES %X", v, 0, 0xf, &data);
        data = (data & 0x0f) | (v << 4);
    }
    else if (g_command == gt::CMD_SETFILTERCUTOFF) {
        gui::slider(WIDTH, "%02X", data, 0, 0xff);
    }
    else if (g_command == gt::CMD_SETMASTERVOL) {
        gui::slider(WIDTH, "%X", data, 0, 15);
    }
    else if (g_command == gt::CMD_SETTEMPO) {
        gui::item_size({ WIDTH, app::BUTTON_HEIGHT });
        if (gui::button("ONLY THIS VOICE", data & 0x80)) {
            data ^= 0x80;
        }
        int v = data & 0x7f;
        gui::slider(WIDTH, "%02X", v, 0, 0x7f);
        data = (data & 0x80) | v;
    }

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
                g_command = row.command;
                g_command_data[g_command] = row.data;
                g_command_row = &row;
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


    draw_order_edit();
    draw_command_edit();

    // piano
    if (piano::draw(&g_follow) && g_edit_mode == EditMode::Pattern && g_recording) {
        assert(g_cursor_pattern_row < patt.len);
        gt::PatternRow& row  = patt.rows[g_cursor_pattern_row];
        row.note  = gt::FIRSTNOTE + piano::note();
        row.instr = piano::instrument();
    }
}

} // namespace song_view
