#include "instrument_view.hpp"
#include "command_edit.hpp"
#include "piano.hpp"
#include "app.hpp"
#include "log.hpp"
#include <cassert>


#define DEBUG_TABLE 0


namespace instrument_view {
namespace {

enum class CursorSelect {
    Adsr,
    GateTimer,
    FirstWave,
    Vibrato,
    Table,
};

gt::Song&          g_song              = app::song();
CursorSelect       g_cursor_select     = {};
int                g_table             = 0;
int                g_scroll            = 0;
int                g_cursor_row        = 0;
std::array<int, 4> g_table_scroll      = {};
bool               g_table_debug       = false;
bool               g_draw_share_window = false;


void add_table_row(int table, int pos) {
    // shift instr pointer
    for (gt::Instrument& instr : g_song.instruments) {
        if (instr.ptr[table] > pos + 1) {
            ++instr.ptr[table];
        }
    }

    // shift table jump pointers
    auto& ltable = g_song.ltable[table];
    auto& rtable = g_song.rtable[table];
    for (int i = pos + 1; i < gt::MAX_TABLELEN; ++i) {
        if (ltable[i] == 0xff && rtable[i] >= pos + 1) {
            ++rtable[i];
        }
    }
    // new row
    std::rotate(ltable.begin() + pos, ltable.end() - 1, ltable.end());
    std::rotate(rtable.begin() + pos, rtable.end() - 1, rtable.end());
}


void delete_table_row(int table, int pos) {
    auto& ltable = g_song.ltable[table];
    auto& rtable = g_song.rtable[table];

    // check if we're deleting the jump row
    // in that case, we want to also clear pointers in instruments
    bool is_jump_row = ltable[pos] == 0xff;

    // shift instr pointer
    for (gt::Instrument& instr : g_song.instruments) {
        if (is_jump_row && instr.ptr[table] == pos + 1) {
            instr.ptr[table] = 0; // reset pointer
        }
        else if (instr.ptr[table] > pos + 1) {
            --instr.ptr[table];
        }
    }

    // shift table jump pointers
    for (int i = pos + 1; i < gt::MAX_TABLELEN; ++i) {
        if (ltable[i] == 0xff && rtable[i] > pos + 1) {
            --rtable[i];
        }
    }
    // delete row
    ltable[pos] = 0;
    rtable[pos] = 0;
    std::rotate(ltable.begin() + pos, ltable.begin() + pos + 1, ltable.end());
    std::rotate(rtable.begin() + pos, rtable.begin() + pos + 1, rtable.end());
}


void draw_table_debug() {
    enum class CursorSelect {
        WaveTable,
        PulseTable,
        FilterTable,
        SpeedTable,
    };
    static CursorSelect g_cursor_select = {};

    gt::Instrument& instr = g_song.instruments[piano::instrument()];
    char str[32];

    gui::DrawContext& dc = gui::draw_context();
    ivec2 cursor = gui::cursor();
    enum { RH = 13 };
    int table_page = (app::canvas_height() - cursor.y - app::BUTTON_HEIGHT * 3 - app::TAB_HEIGHT) / RH;

    for (int t = 0; t < 4; ++t) {
        gui::cursor({ 90 * t, cursor.y });

        // table pointers
        gui::item_size({ 72, app::BUTTON_HEIGHT });
        if (instr.ptr[t] > 0) sprintf(str, "%02X", instr.ptr[t]);
        else sprintf(str, "\x01\x01");
        if (gui::button(str)) {
            if (g_cursor_select == CursorSelect(t) && instr.ptr[t] != g_cursor_row + 1) {
                instr.ptr[t] = g_cursor_row + 1;
            }
            else {
                instr.ptr[t] = 0;
            }
        }

        int pos = instr.ptr[t] - 1;
        auto& ltable = g_song.ltable[t];
        auto& rtable = g_song.rtable[t];

        int& scroll = g_table_scroll[t];
        for (int i = 0; i < table_page; ++i) {
            int row = scroll + i;
            uint8_t& lval = ltable[row];
            uint8_t& rval = rtable[row];

            gui::item_size({ 28, RH });
            gui::Box num_box = gui::item_box();
            gui::same_line();
            gui::item_size({ 44, RH });
            gui::Box box = gui::item_box();

            gui::ButtonState state = gui::button_state(box);
            box.pos.x += 1;
            box.size.x -= 2;

            num_box.pos.x += 1;
            num_box.size.x -= 2;
            dc.rgb(color::ROW_NUMBER);
            if (row == pos) {
                dc.rgb(color::BUTTON_ACTIVE);
                dc.fill(num_box);
                dc.rgb(color::WHITE);
            }
            // row number
            sprintf(str, "%02X", row + 1);
            dc.text(num_box.pos + ivec2(5, 3), str);
            dc.rgb(color::BACKGROUND_ROW);
            dc.fill(box);


            if (state == gui::ButtonState::Released) {
                g_cursor_select = CursorSelect(t);
                g_cursor_row    = row;
            }
            if (state != gui::ButtonState::Normal) {
                dc.rgb(color::BUTTON_PRESSED);
                dc.box(box, gui::BoxStyle::Cursor);
            }
            if (CursorSelect(t) == g_cursor_select && row == g_cursor_row) {
                dc.rgb(color::BUTTON_ACTIVE);
                dc.box(box, gui::BoxStyle::Cursor);
            }

            // choose colors
            uint32_t colors[2] = {
                color::WHITE,
                color::WHITE,
            };
            if (t == gt::WTBL) {

                // wave table
                // left side:
                // + 00    no waveform change
                // + 01-0F delay
                // + 10-DF waveform
                // + E0-EF inaudible waveform 00-0F
                // + F0-FE pattern command
                // + FF    jump
                // right side:
                // + 00-57 relative note
                // + 60-7F negative relative note
                // + 80    no change
                // + 81-DD absolute notes C#0 - B-7

                if (lval >= 0x10 && lval <= 0xef) {
                    colors[0] = color::CMDS[7]; // color like waveform command
                }

                if (lval >= 0xf0 && lval <= 0xfe) {
                    // pattern command
                    int cmd = lval & 0xf;
                    constexpr bool valid_command[] = {
                        0, 1, 1, 1,
                        1, 1, 1, 1,
                        0, 1, 1, 1,
                        1, 0, 0,
                    };
                    if (valid_command[cmd]) {
                        colors[0] = color::CMDS[cmd];
                        colors[1] = color::CMDS[cmd];
                    }
                }
                else if (lval != 0xff) {
                    if (rval > 0 && rval < 0x7f) { // relative pitch
                        colors[1] = color::CMDS[5]; // green
                    }
                    if (rval > 0x80 && rval <= 0xdf) { // absolute pitch
                        colors[1] = color::CMDS[10]; // blue
                    }
                }
            }
            else if (t == gt::PTBL) {
                // 01-7F pulse mod step time/speed
                // 8X-FX set pulsewidth XYY
                // FF    jump
                if (lval > 0 && lval <= 0x7f) { // mod step
                    colors[0] = colors[1] = color::CMDS[5]; // green
                }
                if (lval >= 0x80 && lval < 0xff) { // set
                    colors[0] = colors[1] = color::CMDS[10]; // blue
                }
            }
            else if (t == gt::FTBL) {
                // 00    set cutoff
                // 01-7F filter mod step time/speed
                // 80-F0 set params
                if (lval == 0 && rval > 0) { // set
                    colors[0] = colors[1] = color::CMDS[10]; // blue
                }
                if (lval >= 0x01 && lval <= 0x7f) { // mod
                    colors[0] = colors[1] = color::CMDS[5]; // green
                }
                if (lval >= 0x80 && lval < 0xff) { // params
                    colors[0] = colors[1] = color::CMDS[4];
                }
            }
            else if (t == gt::STBL) {
                if (lval | rval) {
                    colors[0] = colors[1] = color::CMDS[5]; // green
                }
            }


            if (lval == 0xff) {
                sprintf(str, "\x06\x07"); // goto arrow
            }
            else {
                sprintf(str, "%02X", lval);
            }
            dc.rgb(colors[0]);
            auto p = box.pos + ivec2(5, 3);
            dc.text(p, str);
            p.x += 16;
            sprintf(str, "%02X", rval);
            dc.rgb(colors[1]);
            dc.text(p, str);
        }

        gui::cursor({ 90 * t + 72, cursor.y });
        gui::item_size({ 18, RH * table_page + app::BUTTON_HEIGHT });
        gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
        gui::vertical_drag_bar(scroll, 0, gt::MAX_TABLELEN - table_page, table_page);
    }

    gui::cursor({ 0, cursor.y + RH * table_page + app::BUTTON_HEIGHT });

    switch (g_cursor_select) {
    case CursorSelect::WaveTable:
    case CursorSelect::PulseTable:
    case CursorSelect::FilterTable:
    case CursorSelect::SpeedTable: {
        int t = int(g_cursor_select);
        uint8_t* data[] = {
            &g_song.ltable[t][g_cursor_row],
            &g_song.rtable[t][g_cursor_row],
        };
        gui::align(gui::Align::Center);
        for (int n = 0; n < 2; ++ n) {
            for (int i = 0; i < 16; ++i) {
                gui::item_size({ 22 + (i % 2), app::BUTTON_HEIGHT });
                gui::same_line(i > 0);
                sprintf(str, "%X", i);
                if (gui::button(str)) {
                    *data[n] <<= 4;
                    *data[n] |= i;
                }
            }
        }
        break;
    }
    default: assert(false);
    }
}


} // namespace

void reset() {
    g_cursor_select     = {};
    g_table             = 0;
    g_cursor_row        = 0;
    g_scroll            = 0;
    g_table_scroll      = {};
    g_table_debug       = false;
    g_draw_share_window = false;
}

void InstrumentCopyBuffer::copy() {
    instr_num = piano::instrument();
    instr     = g_song.instruments[instr_num];
    ltable    = g_song.ltable;
    rtable    = g_song.rtable;
}


void InstrumentCopyBuffer::paste() const {
    gt::Instrument& dst = g_song.instruments[piano::instrument()];
    for (int t = 0; t < 3; ++t) {
        auto const& src_ltable = ltable[t];
        auto const& src_rtable = rtable[t];
        auto&       dst_ltable = g_song.ltable[t];
        auto&       dst_rtable = g_song.rtable[t];

        // check if current table is shared
        int share_count = 0;
        if (dst.ptr[t] > 0) {
            for (size_t i = 0; i < gt::MAX_INSTR; ++i) {
                gt::Instrument const& in = g_song.instruments[i];
                if (in.ptr[t] == dst.ptr[t]) {
                    ++share_count;
                }
            }
        }

        // delete table if not shared
        if (share_count == 1) {
            int start = dst.ptr[t] - 1;
            for (;;) {
                uint8_t v = dst_ltable[start];
                delete_table_row(t, start);
                if (v == 0xff) break;
            }
        }

        // check if there's a new table
        if (instr.ptr[t] == 0) {
            dst.ptr[t] = 0;
            continue;
        }


        // check if we can borrow new table
        if (instr_num > 0 && instr_num != piano::instrument()) {
            gt::Instrument const& orig_instr = g_song.instruments[instr_num];
            if (orig_instr.ptr[t] > 0) {
                int i = orig_instr.ptr[t] - 1;
                int j = instr.ptr[t] - 1;
                bool borrow = true;
                for (; i < gt::MAX_TABLELEN && j < gt::MAX_TABLELEN; ++i, ++j) {
                    if (dst_ltable[i] != src_ltable[j]) {
                        borrow = false;
                        break;
                    }
                    if (dst_ltable[i] == 0xff) {
                        if (dst_rtable[i] - i == src_rtable[j] - j) break;
                    }
                    if (dst_rtable[i] != src_rtable[j]) {
                        borrow = false;
                        break;
                    }
                }
                if (borrow) {
                    dst.ptr[t] = instr.ptr[t];
                    continue;
                }
            }
        }




        // check if there's enough space
        int start_row = instr.ptr[t] - 1;
        int end_row   = start_row;
        for (; end_row < gt::MAX_TABLELEN; ++end_row) {
            if (src_ltable[end_row] == 0xff) break;
        }
        int new_start_row = g_song.get_table_length(t);
        if (end_row - start_row > gt::MAX_TABLELEN - new_start_row) {
            LOGW("InstrumentCopyBuffer::paste: not enough space in table %d", t);
            dst.ptr[t] = 0;
            continue;
        }

        // insert new table
        int r = new_start_row;
        dst.ptr[t] = r + 1;
        for (int i = start_row; i <= end_row; ++i, ++r) {
            dst_ltable[r] = src_ltable[i];
            dst_rtable[r] = src_rtable[i];
        }
        // fix jump address
        --r;
        assert(dst_ltable[r] == 0xff);
        if (dst_rtable[r] > 0) {
            dst_rtable[r] = dst_rtable[r] - start_row + new_start_row;
        }
    }
    int di = dst.ptr[gt::STBL] - 1;
    g_song.ltable[gt::STBL][di] = 0;
    g_song.rtable[gt::STBL][di] = 0;
    if (instr.ptr[gt::STBL] > 0) {
        int si = instr.ptr[gt::STBL] - 1;
        g_song.ltable[gt::STBL][di] = ltable[gt::STBL][si];
        g_song.rtable[gt::STBL][di] = rtable[gt::STBL][si];
    }
    dst.ad        = instr.ad;
    dst.sr        = instr.sr;
    dst.firstwave = instr.firstwave;
    dst.gatetimer = instr.gatetimer;
    dst.vibdelay  = instr.vibdelay;
    dst.name      = instr.name;
}


void draw() {
    if (DEBUG_TABLE) {
        gui::item_size(app::BUTTON_HEIGHT);
        if (gui::button("", g_table_debug)) g_table_debug ^= 1;
        if (g_table_debug) {
            draw_table_debug();
            return;
        }
        gui::same_line();
    }

    gt::Instrument& instr = g_song.instruments[piano::instrument()];
    gui::DrawContext& dc = gui::draw_context();
    char str[32];

    gui::item_size({ app::CANVAS_WIDTH - app::BUTTON_HEIGHT * 2 - gui::cursor().x, app::BUTTON_HEIGHT });
    gui::input_text(instr.name);

    {
        int adsr_w = 5 * 8;
        int vib_w  = 8 * 8;
        int gate_w = 2 * 8;
        int pad    = app::CANVAS_WIDTH - app::BUTTON_HEIGHT * 2 - adsr_w - vib_w - gate_w * 2;
        pad /= 4;
        adsr_w += pad;
        vib_w  += pad;
        gate_w += pad;

        // adsr
        gui::item_size({ adsr_w, app::BUTTON_HEIGHT });
        gui::button_style(gui::ButtonStyle::PaddedTableCell);
        sprintf(str, "%02X %02X", instr.ad, instr.sr);
        if (gui::button(str, g_cursor_select == CursorSelect::Adsr)) {
            g_cursor_select = CursorSelect::Adsr;
        }

        // vibrato
        gui::same_line();
        gui::item_size({ vib_w, app::BUTTON_HEIGHT });
        {
            int idx = instr.ptr[gt::STBL] - 1;
            int lval = g_song.ltable[gt::STBL][idx];
            int rval = g_song.rtable[gt::STBL][idx];
            if (lval < 0x80) {
                sprintf(str, "%02X %02X %02X", instr.vibdelay, lval, rval);
            }
            else {
                sprintf(str, "%02X %02X \x17%X", instr.vibdelay, lval & 0x7f, rval); // RT speed
            }
            if (gui::button(str, g_cursor_select == CursorSelect::Vibrato)) {
                g_cursor_select = CursorSelect::Vibrato;
            }
        }


        // gate timer
        gui::item_size({ gate_w, app::BUTTON_HEIGHT });
        gui::same_line();
        sprintf(str, "%02X", instr.gatetimer);
        if (gui::button(str, g_cursor_select == CursorSelect::GateTimer)) {
            g_cursor_select = CursorSelect::GateTimer;
        }

        // 1st wave
        gui::same_line();
        sprintf(str, "%02X", instr.firstwave);
        if (gui::button(str, g_cursor_select == CursorSelect::FirstWave)) {
            g_cursor_select = CursorSelect::FirstWave;
        }
    }

    gui::cursor({ app::CANVAS_WIDTH - app::BUTTON_HEIGHT * 2, gui::cursor().y - app::BUTTON_HEIGHT * 2 });
    gui::item_size({ app::BUTTON_HEIGHT * 2, app::BUTTON_HEIGHT });
    static InstrumentCopyBuffer instr_copy_buffer;
    if (gui::button(gui::Icon::Copy)) {
        instr_copy_buffer.copy();
    }
    if (gui::button(gui::Icon::Paste)) {
        instr_copy_buffer.paste();
    }
    gui::cursor({ 0, gui::cursor().y });


    // tables
    enum {
        CN = 28,
        CD = app::CANVAS_WIDTH - CN - app::SCROLL_WIDTH - app::BUTTON_HEIGHT * 2,
    };
    constexpr char const* TABLE_LABELS[] = { "WAVE", "PULSE", "FILTER" };
    gui::item_size({ (app::CANVAS_WIDTH - app::BUTTON_HEIGHT * 2) / 3, app::BUTTON_HEIGHT });
    for (int t = 0; t < 3; ++t) {
        gui::button_style(instr.ptr[t] > 0 ? gui::ButtonStyle::Tab : gui::ButtonStyle::ShadedTab );
        if (gui::button(TABLE_LABELS[t], t == g_table)) {
            g_cursor_select = CursorSelect::Table;
            g_table = t;
        }
        gui::same_line();
    }
    gui::button_style(gui::ButtonStyle::Normal);

    // instr sharing
    size_t      share_count       = 0;
    if (instr.ptr[g_table] > 0) {
        for (size_t i = 0; i < gt::MAX_INSTR; ++i) {
            gt::Instrument const& in = g_song.instruments[i];
            if (in.ptr[g_table] == instr.ptr[g_table]) {
                ++share_count;
            }
        }
    }
    gui::item_size({ app::BUTTON_HEIGHT * 2, app::BUTTON_HEIGHT });
    if (gui::button(gui::Icon::Share, share_count >= 2)) {
        g_draw_share_window = true;
    }
    gui::item_size(app::CANVAS_WIDTH);
    gui::separator();

    ivec2 cursor      = gui::cursor();
    int   table_space = app::canvas_height() - cursor.y - piano::HEIGHT - app::BUTTON_HEIGHT * 4 - gui::FRAME_WIDTH - 2;
    int   table_page  = table_space / app::MAX_ROW_HEIGHT;

    auto& ltable = g_song.ltable[g_table];
    auto& rtable = g_song.rtable[g_table];
    int start_row = 0;
    int end_row = 0;
    if (instr.ptr[g_table] > 0) {
        start_row = instr.ptr[g_table] - 1;
        end_row   = start_row + g_song.get_table_part_length(g_table, start_row) - 1;
    }
    // NOTE: number of visible rows (excluding the jump row)
    int len = end_row - start_row;

    g_scroll = clamp(g_scroll, 0, len - table_page);
    g_cursor_row = std::min(g_cursor_row, len - 1);
    g_cursor_row = std::max(g_cursor_row, 0);

    gui::cursor({ 0, cursor.y + 1 });
    for (int i = 0; i < table_page; ++i) {
        int r = i + g_scroll;
        sprintf(str, "%02X", r);
        gui::item_size({ CN, app::MAX_ROW_HEIGHT });
        gui::Box box = gui::item_box();
        dc.rgb(color::ROW_NUMBER);
        dc.text(box.pos + 6, str);
        gui::same_line();

        gui::item_size({ CD, app::MAX_ROW_HEIGHT });
        box = gui::item_box();
        box.pos.x += 1;
        box.size.x -= 2;

        // draw cursor if table is empty
        if (g_cursor_select == CursorSelect::Table && len == 0 && r == 0) {
            dc.rgb(color::BUTTON_ACTIVE);
            dc.box(box, gui::BoxStyle::Cursor);
        }

        if (r >= len) continue;

        dc.rgb(color::BACKGROUND_ROW);
        dc.fill(box);

        // loop marker
        if (start_row + r + 1 == rtable[end_row]) {
            dc.rgb(color::BUTTON_PRESSED);
            dc.text(box.pos + ivec2(box.size.x - 8, 6), "\x05");
        }

        gui::ButtonState state = gui::button_state(box);
        if (state == gui::ButtonState::Released) {
            g_cursor_row = r;
            g_cursor_select = CursorSelect::Table;
        }
        if (state != gui::ButtonState::Normal) {
            dc.rgb(color::BUTTON_PRESSED);
            dc.box(box, gui::BoxStyle::Cursor);
        }
        if (g_cursor_select == CursorSelect::Table && g_cursor_row == r) {
            dc.rgb(color::BUTTON_ACTIVE);
            dc.box(box, gui::BoxStyle::Cursor);
        }

        uint8_t& lval = ltable[start_row + r];
        uint8_t& rval = rtable[start_row + r];
        assert(lval != 0xff);

        ivec2 p = box.pos + ivec2(5, 6);
        char* s = str;
        if (g_table == gt::WTBL) {
            // left side:
            // + 00-0F delay
            // + 10-DF waveform
            // + E0-EF inaudible waveform 00-0F
            // + F0-FE pattern command
            // right side:
            // + 00-5F relative note
            // + 60-7F negative relative note
            // + 80    no change
            // + 81-DD absolute notes C#0 - B-7
            if (lval >= 0xf0 && lval <= 0xfe) {
                int cmd = lval & 0xf;
                sprintf(s, "COMMAND \x81%c%X%02X", cmd, cmd, rval);
            }
            else {
                if (lval < 0x10) {
                    s += sprintf(s, "DELAY \xf3%X ", lval);
                }
                else {
                    uint8_t v = lval;
                    if (v >= 0xe0) v -= 0xe0;
                    s += sprintf(s, "WAVE \xf2%02X ", v);
                }
                if (rval >= 0 && rval <= 0x7f) { // relative pitch
                    int v = rval < 0x60 ? rval : rval - 0x80;
                    s += sprintf(s, "\xf4%+03d ", v);
                }
                if (rval > 0x80 && rval <= 0xdf) { // absolute pitch
                    s += sprintf(s, "\xf5 %02d", rval - 0x80);
                }
            }
        }
        else if (g_table == gt::PTBL) {
            // 01-7F pulse mod step time/speed
            // 8X-FX set pulsewidth XYY
            if (lval < 0x80) {
                int v = int8_t(rval);
                s += sprintf(s, "MOD \xf3%02X \xf4%c%02X", lval, "+-"[v < 0], abs(v));
            }
            else {
                int v = ((lval & 0xf) << 8) | rval;
                s += sprintf(s, "SET    \xf5%03X", v);
            }
        }
        else if (g_table == gt::FTBL) {
            // 00    set cutoff
            // 01-7F filter mod step time/speed
            // 80-F0 set params
            if (lval == 0) { // set
                s += sprintf(s, "SET CUTOFF     \xf5%02X", rval);
            }
            if (lval >= 0x01 && lval <= 0x7f) { // mod
                int v = int8_t(rval);
                s += sprintf(s, "MOD CUTOFF \xf3%02X \xf4%c%02X", lval, "+-"[v < 0], abs(v));
            }
            if (lval >= 0x80 && lval < 0xff) { // params
                s += sprintf(s, "PARAMS  ");
                *s++ = (rval & 0x1) ? 0xf1 : 0xf7;
                *s++ = '1';
                *s++ = (rval & 0x2) ? 0xf1 : 0xf7;
                *s++ = '2';
                *s++ = (rval & 0x4) ? 0xf1 : 0xf7;
                *s++ = '3';
                *s++ = ' ';
                *s++ = (lval & 0x10) ? 0xf1 : 0xf7;
                *s++ = '\x14';
                *s++ = (lval & 0x20) ? 0xf1 : 0xf7;
                *s++ = '\x15';
                *s++ = (lval & 0x40) ? 0xf1 : 0xf7;
                *s++ = '\x16';
                s += sprintf(s, " \xf1%X", rval >> 4);
            }
        }

        dc.rgb(color::WHITE);
        dc.text(p, str);
    }
    int y = gui::cursor().y + 1; // 1px padding

    // scrolling
    gui::cursor({ CN + CD, cursor.y });
    gui::item_size({ app::SCROLL_WIDTH, app::MAX_ROW_HEIGHT * table_page + 2 });
    gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
    gui::vertical_drag_bar(g_scroll, 0, len - table_page, table_page);
    gui::same_line();
    gui::separator();
    gui::same_line(false);


    // buttons
    int num_free_rows = gt::MAX_TABLELEN - g_song.get_table_length(g_table);
    if (g_cursor_select == CursorSelect::Table) {
        gui::cursor({ app::CANVAS_WIDTH - 55, cursor.y });

        gui::item_size({ 55, app::BUTTON_HEIGHT });
        gui::disabled(num_free_rows <= 0 + (len == 0));
        if (gui::button(gui::Icon::AddRowAbove)) {
            // add jump
            if (len == 0) {
                assert(num_free_rows >= 2);
                start_row = gt::MAX_TABLELEN - num_free_rows;
                instr.ptr[g_table] = start_row + 1;
                add_table_row(g_table, start_row);
                ltable[start_row] = 0xff;
                ++len;
            }
            add_table_row(g_table, start_row + g_cursor_row);
            if (g_table == gt::PTBL) ltable[start_row + g_cursor_row] = 0x88; // set pw
            ++len;
        }
        gui::disabled(len == 0 || num_free_rows == 0);
        if (gui::button(gui::Icon::AddRowBelow)) {
            ++g_cursor_row;
            ++len;
            add_table_row(g_table, start_row + g_cursor_row);
            if (g_table == gt::PTBL) ltable[start_row + g_cursor_row] = 0x88; // set pw
        }

        gui::disabled(g_cursor_row >= len);
        if (gui::button(gui::Icon::DeleteRow)) {
            delete_table_row(g_table, start_row + g_cursor_row);
            --len;
            --end_row;
            if (len == 0) {
                // delete jump row
                delete_table_row(g_table, start_row);
            }
            else {
                // clamp jump pointer
                auto& loop = rtable[end_row];
                if (loop > 0) loop = clamp<int>(loop, start_row + 1, end_row);
                g_cursor_row = std::min(g_cursor_row, len - 1);
            }
        }
        bool is_loop = rtable[end_row] == start_row + g_cursor_row + 1;
        if (gui::button(gui::Icon::JumpBack)) {
            if (is_loop) rtable[end_row] = 0;
            else rtable[end_row] = start_row + g_cursor_row + 1;
        }
        gui::disabled(false);
        gui::separator();
    }

    // draw box
    gui::cursor({ 0, y });
    gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
    gui::separator();

    if (g_cursor_select == CursorSelect::Adsr) {
        int adsr[] = {
            instr.ad >> 4,
            instr.ad & 0xf,
            instr.sr >> 4,
            instr.sr & 0xf,
        };
        constexpr char const* LABELS[] = { "ATTACK  %X", "DECAY   %X", "SUSTAIN %X", "RELEASE %X" };
        for (int i = 0; i < 4; ++i) {
            gui::slider(app::CANVAS_WIDTH, LABELS[i], adsr[i], 0, 0xf);
        }
        instr.ad = (adsr[0] << 4) | adsr[1];
        instr.sr = (adsr[2] << 4) | adsr[3];
    }
    else if (g_cursor_select == CursorSelect::GateTimer) {
        int v = instr.gatetimer & 0x3f;
        gui::slider(app::CANVAS_WIDTH, "GATE TIMER %2X", v, 1, 0x3f, &instr.gatetimer);
        instr.gatetimer = (instr.gatetimer & 0xc0) | v;

        gui::item_size({ 12 + 8 * 13, app::BUTTON_HEIGHT });
        gui::text("HARD RESTART ");
        gui::same_line();
        gui::item_size({ (app::CANVAS_WIDTH - gui::cursor().x) / 2, app::BUTTON_HEIGHT });
        gui::button_style(gui::ButtonStyle::RadioLeft);
        if (gui::button("OFF", instr.gatetimer & 0x80)) {
            instr.gatetimer |= 0x80;
        }
        gui::same_line();
        gui::button_style(gui::ButtonStyle::RadioRight);
        if (gui::button("ON", !(instr.gatetimer & 0x80))) {
            instr.gatetimer &= ~0x80;
        }

        gui::item_size({ 12 + 8 * 13, app::BUTTON_HEIGHT });
        gui::text("DISABLE GATE ");
        gui::same_line();
        gui::item_size({ (app::CANVAS_WIDTH - gui::cursor().x) / 2, app::BUTTON_HEIGHT });
        gui::button_style(gui::ButtonStyle::RadioLeft);
        if (gui::button("OFF", instr.gatetimer & 0x40)) {
            instr.gatetimer |= 0x40;
        }
        gui::same_line();
        gui::button_style(gui::ButtonStyle::RadioRight);
        if (gui::button("ON", !(instr.gatetimer & 0x40))) {
            instr.gatetimer &= ~0x40;
        }
    }
    else if (g_cursor_select == CursorSelect::FirstWave) {
        gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
        gui::text("FIRST WAVE");
        gui::item_size({ app::CANVAS_WIDTH / 4, app::BUTTON_HEIGHT });
        gui::button_style(gui::ButtonStyle::RadioLeft);
        if (gui::button("WAVE", instr.firstwave > 0 && instr.firstwave < 0xfe)) {
            instr.firstwave = 0x09;
        }
        gui::same_line();
        gui::button_style(gui::ButtonStyle::RadioCenter);
        if (gui::button("GATE ON", instr.firstwave == 0xfe)) {
            instr.firstwave = 0xfe;
        }
        gui::same_line();
        if (gui::button("GATE OFF", instr.firstwave == 0xff)) {
            instr.firstwave = 0xff;
        }
        gui::same_line();
        gui::button_style(gui::ButtonStyle::RadioRight);
        if (gui::button("NO CHANGE", instr.firstwave == 0x00)) {
            instr.firstwave = 0x00;
        }
        gui::button_style(gui::ButtonStyle::Normal);
        if (instr.firstwave > 0 && instr.firstwave < 0xfe) {
            // show wave buttons
            uint8_t v = instr.firstwave;
            for (int i = 0; i < 8; ++i) {
                gui::item_size({ app::CANVAS_WIDTH / 8, app::BUTTON_HEIGHT });
                int mask = 0x80 >> i;
                int icon = int(gui::Icon::Noise) + i;
                if (gui::button(gui::Icon(icon), v & mask)) {
                    v ^= mask;
                }
                gui::same_line();
            }
            gui::same_line(false);
            instr.firstwave = v;
        }
    }
    else if (g_cursor_select == CursorSelect::Vibrato) {
        assert(instr.ptr[gt::STBL] > 0);
        uint8_t& lval = g_song.ltable[gt::STBL][instr.ptr[gt::STBL] - 1];
        uint8_t& rval = g_song.rtable[gt::STBL][instr.ptr[gt::STBL] - 1];

        gui::slider(app::CANVAS_WIDTH, "VIBRATO DELAY %02X", instr.vibdelay, 0, 255);
        int v = lval & 0x7f;
        gui::slider(app::CANVAS_WIDTH, "STEPS %02X", v, 0, 0x7f);
        lval = (lval & 0x80) | v;

        gui::item_size({ app::CANVAS_WIDTH / 2, app::BUTTON_HEIGHT });
        gui::button_style(gui::ButtonStyle::RadioLeft);
        if (gui::button("PRECALCULATED", !(lval & 0x80)) && (lval & 0x80)) {
            int note = piano::note();
            uint16_t speed = gt::get_freq(note + 1) - gt::get_freq(note);
            rval = std::min(255, speed >> rval);
            lval &= ~0x80;
        }
        gui::same_line();
        gui::button_style(gui::ButtonStyle::RadioRight);
        if (gui::button("NOTE-INDEPENDENT", lval & 0x80) && !(lval & 0x80)) {
            rval = 2;
            lval |= 0x80;
        }
        if (lval & 0x80) {
            gui::slider(app::CANVAS_WIDTH, "SHIFT  %X", rval, 0, 8);
        }
        else {
            gui::slider(app::CANVAS_WIDTH, "SPEED %02X", rval, 0, 0xff);
        }

    }
    else if (g_cursor_select == CursorSelect::Table && g_cursor_row < len) {
        uint8_t& lval = ltable[start_row + g_cursor_row];
        uint8_t& rval = rtable[start_row + g_cursor_row];
        if (g_table == gt::WTBL) {
            // left side:
            // + 00-0F delay
            // + 10-DF waveform
            // + E0-EF inaudible waveform 00-0F
            // + F0-FE pattern command
            // + FF    jump
            int mode = (lval <= 0x0f) ? 1 : (lval <= 0xef) ? 0 : 2;
            gui::item_size({ app::CANVAS_WIDTH / 3, app::BUTTON_HEIGHT });
            gui::button_style(gui::ButtonStyle::RadioLeft);
            if (gui::button("WAVE", mode == 0) && mode != 0) {
                mode = 0;
                lval = 0x11;
            }
            gui::same_line();
            gui::button_style(gui::ButtonStyle::RadioCenter);
            if (gui::button("DELAY", mode == 1) && mode != 1) {
                mode = 1;
                lval = 0;
            }
            gui::same_line();
            gui::button_style(gui::ButtonStyle::RadioRight);
            if (gui::button("COMMAND", mode == 2) && mode != 2) {
                mode = 2;
                lval = 0xf1;
                rval = 0x00;
                command_edit::init(command_edit::Location::WaveTable, lval & 0xf, rval, [&lval, &rval](uint8_t cmd, uint8_t data) {
                    lval = cmd | 0xf0;
                    rval = data;
                });
            }

            gui::button_style(gui::ButtonStyle::Normal);
            if (mode == 0) {
                // wave
                uint8_t v = lval;
                if (v >= 0xe0) v -= 0xe0;
                for (int i = 0; i < 8; ++i) {
                    gui::item_size({ app::CANVAS_WIDTH / 8, app::BUTTON_HEIGHT });
                    int mask = 0x80 >> i;
                    int icon = int(gui::Icon::Noise) + i;
                    if (gui::button(gui::Icon(icon), v & mask)) {
                        v ^= mask;
                        if ((v & 0xe0) == 0xe0) v ^= (i == 0) ? 0x20 : 0x80;
                    }
                    gui::same_line();
                }
                gui::same_line(false);
                if (v < 0x10) v += 0xe0;
                lval = v;
            }
            else if (mode == 1) {
                // delay
                int v = lval;
                gui::slider(app::CANVAS_WIDTH, "  %X", v, 0, 0xf, &lval);
                lval = v;
            }
            if (mode != 2) {
                // right side:
                // + 00-5F relative note
                // + 60-7F negative relative note
                // + 80    do nothing
                // + 81-DD absolute notes C#0 - B-7
                mode = (rval < 0x80) ? 0 : (rval > 0x80) ? 1 : 2;
                gui::item_size({ app::CANVAS_WIDTH / 3, app::BUTTON_HEIGHT });
                gui::button_style(gui::ButtonStyle::RadioLeft);
                if (gui::button("RELATIVE \x09", mode == 0) && mode != 0) {
                    mode = 0;
                    rval = 0;
                }
                gui::same_line();
                gui::button_style(gui::ButtonStyle::RadioCenter);
                if (gui::button("ABSOLUTE \x09", mode == 1) && mode != 1) {
                    mode = 1;
                    rval = 0x80 + 48;
                }
                gui::same_line();
                gui::button_style(gui::ButtonStyle::RadioRight);
                if (gui::button("NO CHANGE", mode == 2) && mode != 2) {
                    mode = 2;
                    rval = 0x80;
                }

                if (mode == 0) {
                    int v = rval < 0x60 ? rval : rval - 0x80;
                    sprintf(str, "%+03d", v);
                    gui::slider(app::CANVAS_WIDTH, str, v, -32, 60, &rval);
                    rval = v >= 0 ? v : v + 0x80;
                }
                else if (mode == 1) {
                    int v = rval - 0x80;
                    gui::slider(app::CANVAS_WIDTH, " %02d", v, 1, 95, &rval);
                    rval = v + 0x80;
                }
            }
            else {
                // command
                gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
                if (gui::button("EDIT COMMAND")) {
                    command_edit::init(command_edit::Location::WaveTable, lval & 0xf, rval, [&lval, &rval](uint8_t cmd, uint8_t data) {
                        lval = cmd | 0xf0;
                        rval = data;
                    });
                }
            }
        }
        else if (g_table == gt::PTBL) {
            // 01-7F pulse mod step time/speed
            // 8X-FX set pulsewidth XYY
            int mode = lval >= 0x80 ? 0 : 1;
            gui::item_size({ app::CANVAS_WIDTH / 2, app::BUTTON_HEIGHT });
            gui::button_style(gui::ButtonStyle::RadioLeft);
            if (gui::button("SET PULSE WIDTH", mode == 0) && mode != 0) {
                mode = 0;
                lval = 0x88;
                rval = 0x00;
            }
            gui::same_line();
            gui::button_style(gui::ButtonStyle::RadioRight);
            if (gui::button("MOD PULSE WIDTH", mode == 1) && mode != 1) {
                mode = 1;
                lval = 0x7f;
                rval = 0x20;
            }
            if (mode == 0) {
                // set pw
                int v = ((lval & 0xf) << 8) | rval;
                gui::slider(app::CANVAS_WIDTH, "%03X", v, 0, 0xfff, &rval);
                lval = 0x80 | (v >> 8);
                rval = v & 0xff;
            }
            else {
                // step pw
                gui::slider(app::CANVAS_WIDTH, "STEPS  %02X", lval, 1, 0x7f);
                int v = int8_t(rval);
                sprintf(str, "SPEED %c%02X", "+-"[v < 0], abs(v));
                gui::slider(app::CANVAS_WIDTH, str, v, -128, 127, &rval);
                rval = v;
            }

        }
        else if (g_table == gt::FTBL) {
            // 00    set cutoff
            // 01-7F filter mod step time/speed
            // 80-F0 set params

            int mode = (lval >= 0x80) ? 0 : (lval == 0x00) ? 1 : 2;
            gui::item_size({ app::CANVAS_WIDTH / 3, app::BUTTON_HEIGHT });
            gui::button_style(gui::ButtonStyle::RadioLeft);
            if (gui::button("SET PARAMS", mode == 0) && mode != 0) {
                mode = 0;
                lval = 0x90;
                rval = 0xF1;
            }
            gui::same_line();
            gui::button_style(gui::ButtonStyle::RadioCenter);
            if (gui::button("SET CUTOFF", mode == 1) && mode != 1) {
                mode = 1;
                lval = 0x00;
                rval = 0x20;
            }
            gui::same_line();
            gui::button_style(gui::ButtonStyle::RadioRight);
            if (gui::button("MOD CUTOFF", mode == 2) && mode != 2) {
                mode = 2;
                lval = 0x01;
                rval = 0x00;
            }

            if (mode == 0) {
                // set params
                gui::button_style(gui::ButtonStyle::Normal);
                gui::item_size({ app::CANVAS_WIDTH / 3, app::BUTTON_HEIGHT });

                if (gui::button("VOICE 1", rval & 0x1)) rval ^= 0x1;
                gui::same_line();
                if (gui::button("VOICE 2", rval & 0x2)) rval ^= 0x2;
                gui::same_line();
                if (gui::button("VOICE 3", rval & 0x4)) rval ^= 0x4;

                if (gui::button(gui::Icon::Lowpass, lval & 0x10)) lval ^= 0x10;
                gui::same_line();
                if (gui::button(gui::Icon::Bandpass, lval & 0x20)) lval ^= 0x20;
                gui::same_line();
                if (gui::button(gui::Icon::Highpass, lval & 0x40)) lval ^= 0x40;

                int v = rval >> 4;
                gui::slider(app::CANVAS_WIDTH, "RES %X", v, 0, 0xf, &rval);
                rval = (rval & 0x0f) | (v << 4);
            }
            else if (mode == 1) {
                // set cutoff
                gui::slider(app::CANVAS_WIDTH, "%02X", rval, 0, 0xff);
            }
            else {
                // step cutoff
                gui::slider(app::CANVAS_WIDTH, "STEPS  %02X", lval, 1, 0x7f);
                int v = int8_t(rval);
                sprintf(str, "SPEED %c%02X", "+-"[v < 0], abs(v));
                gui::slider(app::CANVAS_WIDTH, str, v, -0x20, 0x20, &rval);
                rval = v;
            }
        }
    }



    // table sharing window
    if (g_draw_share_window) {
        int space = app::canvas_height() - 12 - app::BUTTON_HEIGHT * 2;
        int row_h = std::min<int>(space / 32, app::BUTTON_HEIGHT);
        enum {
            COL_W = 12 + 8 * 18,
        };
        gui::begin_window({ COL_W * 2, row_h * 32 + app::BUTTON_HEIGHT * 2 + gui::FRAME_WIDTH * 2 });
        gui::item_size({ COL_W * 2, app::BUTTON_HEIGHT });
        gui::text("%s TABLE SHARING", TABLE_LABELS[g_table]);
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
            gt::Instrument const& in = g_song.instruments[i];
            sprintf(str, "%02X %s", i, in.name.data());
            gui::disabled(in.ptr[g_table] == 0);
            if (gui::button(str, in.ptr[g_table] > 0 && in.ptr[g_table] == instr.ptr[g_table])) {
                g_draw_share_window = false;
                if (instr.ptr[g_table] != in.ptr[g_table] && share_count == 1) {
                    // TODO: confirmation dialog
                    // delete table
                    for (int i = 0; i <= len; ++i) delete_table_row(g_table, start_row);
                }
                instr.ptr[g_table] = in.ptr[g_table];
            }
        }
        gui::align(gui::Align::Center);
        gui::button_style(gui::ButtonStyle::Normal);
        gui::item_size(COL_W * 2);
        gui::disabled(false);
        gui::separator();

        gui::item_size({ COL_W * 2 / 3, app::BUTTON_HEIGHT });
        gui::disabled(share_count < 2 || num_free_rows < (len + 1));
        if (gui::button("CLONE")) {
            g_draw_share_window = false;
            // copy
            int new_start_row = ltable.size() - num_free_rows;
            for (int i = 0; i <= len; ++i) {
                ltable[new_start_row + i] = ltable[start_row + i];
                rtable[new_start_row + i] = rtable[start_row + i];
            }
            assert(ltable[end_row] == 0xff);

            // fix jump address
            if (rtable[end_row] > 0) {
                rtable[new_start_row + len] += new_start_row - start_row;
            }
            instr.ptr[g_table] = new_start_row + 1;
        }
        gui::same_line();
        gui::disabled(instr.ptr[g_table] == 0);
        if (gui::button("DELETE")) {
            g_draw_share_window = false;
            if (share_count == 1) {
                // delete table
                for (int i = 0; i <= len; ++i) delete_table_row(g_table, start_row);
            }
            instr.ptr[g_table] = 0;
        }
        gui::disabled(false);
        gui::same_line();
        if (gui::button("CLOSE")) g_draw_share_window = false;
        gui::end_window();
    }


    command_edit::draw();

    piano::draw();
}


} // namespace instrument_view
