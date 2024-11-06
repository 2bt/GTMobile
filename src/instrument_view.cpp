#include "instrument_view.hpp"
#include "settings_view.hpp"
#include "piano.hpp"
#include "app.hpp"
#include "gui.hpp"
#include <cassert>


namespace instrument_view {
namespace {

bool g_easy_mode = true;

gt::Song& g_song = app::song();

enum class CursorSelect {
    Adsr,
    GateTimer,
    FirstWave,
    //...
    Table,
};
CursorSelect g_cursor_select = {};
int          g_table         = 0;
int          g_cursor_row    = 0;
int          g_scroll        = 0;


void add_table_row(int pos) {
    // shift instr pointer
    for (gt::Instrument& instr : g_song.instruments) {
        if (instr.ptr[g_table] > pos + 1) {
            ++instr.ptr[g_table];
        }
    }

    // shift pointers in wave table
    auto& wav_ltable = g_song.ltable[gt::WTBL];
    auto& wav_rtable = g_song.rtable[gt::WTBL];
    for (int i = 0; i < gt::MAX_TABLELEN; ++i) {
        if (wav_ltable[i] == 8 + g_table && wav_rtable[i] > pos + 1) {
            ++wav_rtable[i];
        }
    }

    // shift pointers in patterns
    for (gt::Pattern& pat : g_song.patterns) {
        for (gt::PatternRow& r : pat.rows) {
            if (r.command == 8 + g_table && r.data > pos + 1) {
                ++r.data;
            }
        }
    }

    // shift table jump pointers
    auto& ltable = g_song.ltable[g_table];
    auto& rtable = g_song.rtable[g_table];
    for (int i = pos + 1; i < gt::MAX_TABLELEN; ++i) {
        if (ltable[i] == 0xff && rtable[i] >= pos + 1) {
            ++rtable[i];
        }
    }
    // new row
    std::rotate(ltable.begin() + pos, ltable.end() - 1, ltable.end());
    std::rotate(rtable.begin() + pos, rtable.end() - 1, rtable.end());
}

void delete_table_row(int pos) {
    auto& ltable = g_song.ltable[g_table];
    auto& rtable = g_song.rtable[g_table];

    // check if we're deleting the jump row
    // in that case, we want to also clear pointers in instruments, wave table, and patterns
    bool is_jump_row = ltable[pos] == 0xff;
    // TODO

    // shift instr pointer
    for (gt::Instrument& instr : g_song.instruments) {
        if (is_jump_row && instr.ptr[g_table] == pos + 1) {
            instr.ptr[g_table] = 0; // reset pointer
        }
        else if (instr.ptr[g_table] > pos + 1) {
            --instr.ptr[g_table];
        }
    }

    // shift pointers in wave table
    auto& wav_ltable = g_song.ltable[gt::WTBL];
    auto& wav_rtable = g_song.rtable[gt::WTBL];
    for (int i = 0; i < gt::MAX_TABLELEN; ++i) {
        if (wav_ltable[i] == 8 + g_table && wav_rtable[i] > pos + 1) {
            --wav_rtable[i];
        }
    }

    // shift pointers in patterns
    for (gt::Pattern& pat : g_song.patterns) {
        for (gt::PatternRow& r : pat.rows) {
            if (r.command == 8 + g_table && r.data > pos + 1) {
                --r.data;
            }
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




void draw_easy() {
    gt::Instrument& instr = g_song.instruments[piano::instrument()];
    char str[32];

    gui::same_line();
    gui::align(gui::Align::Left);
    gui::item_size({ 16 * 8 + 12, app::BUTTON_HEIGHT });
    gui::input_text(instr.name);
    gui::align(gui::Align::Center);

    // adsr
    gui::button_style(gui::ButtonStyle::TableCell);
    gui::same_line();
    gui::item_size({ 12 + 4 * 8, app::BUTTON_HEIGHT });
    sprintf(str, "%02X%02X", instr.ad, instr.sr);
    if (gui::button(str, g_cursor_select == CursorSelect::Adsr)) {
        g_cursor_select = CursorSelect::Adsr;
    }

    // gate timer
    gui::item_size({ 12 + 2 * 8, app::BUTTON_HEIGHT });
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



    // tables
    enum {
        CW_NUM = 28,
        CW_DATA = app::CANVAS_WIDTH - CW_NUM - app::SCROLL_WIDTH - app::BUTTON_HEIGHT * 2,
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
    static bool draw_share_window = false;
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
        draw_share_window = true;
    }

    ivec2 cursor      = gui::cursor();
    int   table_space = app::canvas_height() - cursor.y - piano::HEIGHT - app::BUTTON_HEIGHT * 4 - gui::FRAME_WIDTH - 2;
    int   table_page  = table_space / app::MAX_ROW_HEIGHT;
    int   text_offset = (app::MAX_ROW_HEIGHT - 7) / 2;

    auto& ltable = g_song.ltable[g_table];
    auto& rtable = g_song.rtable[g_table];
    int start_row = 0;
    int end_row = 0;
    int len = 0;
    if (instr.ptr[g_table] > 0) {
        start_row = instr.ptr[g_table] - 1;
        end_row = start_row;
        for (; end_row < gt::MAX_TABLELEN; ++end_row) {
            if (ltable[end_row] == 0xff) break;
        }
        len = end_row - start_row;
        // TODO: verify in Song::load
        assert(end_row < gt::MAX_TABLELEN);
        assert(len > 0);
        int loop = rtable[end_row];
        assert(loop == 0 || (loop - 1 >= start_row && loop - 1 < end_row));
    }

    // find free row
    int num_free_rows = 0;
    while (num_free_rows < int(ltable.size()) &&
           ltable[ltable.size() - num_free_rows - 1] == 0 &&
           rtable[ltable.size() - num_free_rows - 1] == 0)
    {
        ++num_free_rows;
    }

    g_scroll = clamp(g_scroll, 0, len - table_page);
    g_cursor_row = std::min(g_cursor_row, len - 1);
    g_cursor_row = std::max(g_cursor_row, 0);

    gui::cursor({ 0, cursor.y + 1 });
    gui::DrawContext& dc = gui::draw_context();
    for (int i = 0; i < table_page; ++i) {
        int r = i + g_scroll;
        sprintf(str, "%02X", r);
        gui::item_size({ CW_NUM, app::MAX_ROW_HEIGHT });
        gui::Box box = gui::item_box();
        dc.rgb(color::ROW_NUMBER);
        dc.text(box.pos + ivec2(6, text_offset), str);
        gui::same_line();

        gui::item_size({ CW_DATA, app::MAX_ROW_HEIGHT });
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
            dc.rgb(color::BUTTON_HELD);
            dc.text(box.pos + ivec2(box.size.x - 8, text_offset), "\x05");
        }

        gui::ButtonState state = gui::button_state(box);
        if (state == gui::ButtonState::Released) {
            g_cursor_row = r;
            g_cursor_select = CursorSelect::Table;
        }
        if (state != gui::ButtonState::Normal) {
            dc.rgb(color::BUTTON_HELD);
            dc.box(box, gui::BoxStyle::Cursor);
        }
        if (g_cursor_select == CursorSelect::Table && g_cursor_row == r) {
            dc.rgb(color::BUTTON_ACTIVE);
            dc.box(box, gui::BoxStyle::Cursor);
        }

        uint8_t& lval = ltable[start_row + r];
        uint8_t& rval = rtable[start_row + r];
        assert(lval != 0xff);

        // chose colors
        uint32_t colors[2] = {
            color::WHITE,
            color::WHITE,
        };
        if (g_table == gt::WTBL) {
            // left side:
            // + 00-0F delay
            // + 10-DF waveform
            // + E0-EF inaudible waveform 00-0F
            // + F0-FE pattern command
            // + FF    jump
            // right side:
            // + 00-5F relative note
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
                    1, 1, 0,
                };
                if (valid_command[cmd]) {
                    colors[0] = color::CMDS[cmd];
                    colors[1] = color::CMDS[cmd];
                }
            }
            else if (lval != 0xff) {
                if (rval > 0 && rval <= 0x7f) { // relative pitch
                    colors[1] = color::CMDS[5]; // green
                }
                if (rval > 0x80 && rval <= 0xdf) { // absolute pitch
                    colors[1] = color::CMDS[10]; // blue
                }
            }
        }
        else if (g_table == gt::PTBL) {
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
        else if (g_table == gt::FTBL) {
            // 00    set cutoff
            // 01-7F filter mod step time/speed
            // 80-F0 set params
            if (lval == 0) { // set
                colors[0] = colors[1] = color::CMDS[10]; // blue
            }
            if (lval >= 0x01 && lval <= 0x7f) { // mod
                colors[0] = colors[1] = color::CMDS[5]; // green
            }
            if (lval >= 0x80 && lval < 0xff) { // params
                colors[0] = colors[1] = color::CMDS[4];
            }
        }
        else if (g_table == gt::STBL) {
            if (lval | rval) {
                colors[0] = colors[1] = color::CMDS[5]; // green
            }
        }

        sprintf(str, "%02X", lval);
        dc.rgb(colors[0]);
        ivec2 p = box.pos + ivec2(5, text_offset);
        dc.text(p, str);
        p.x += 20;
        sprintf(str, "%02X", rval);
        dc.rgb(colors[1]);
        dc.text(p, str);

    }
    int y = gui::cursor().y + 1; // 1px padding

    // scrolling
    gui::cursor({ CW_NUM + CW_DATA, cursor.y });
    gui::item_size({ app::SCROLL_WIDTH, app::MAX_ROW_HEIGHT * table_page + 2 });
    gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
    gui::vertical_drag_bar(g_scroll, 0, len - table_page, table_page);


    // table buttons
    if (g_cursor_select == CursorSelect::Table) {
        gui::cursor({ app::CANVAS_WIDTH - app::BUTTON_HEIGHT * 2, cursor.y });

        gui::item_size({ app::BUTTON_HEIGHT * 2, app::BUTTON_HEIGHT });
        gui::disabled(len == 0 ? num_free_rows <= 1 : num_free_rows == 0);
        if (gui::button(gui::Icon::AddRowAbove)) {
            // add jump
            if (len == 0) {
                assert(num_free_rows >= 2);
                start_row = ltable.size() - num_free_rows;
                instr.ptr[g_table] = start_row + 1;
                add_table_row(start_row);
                ltable[start_row] = 0xff;
                ++len;
            }
            add_table_row(start_row + g_cursor_row);
            if (g_table == gt::PTBL) ltable[start_row + g_cursor_row] = 0x88; // set pw
            ++len;
        }
        gui::disabled(len == 0 || num_free_rows == 0);
        if (gui::button(gui::Icon::AddRowBelow)) {
            ++g_cursor_row;
            ++len;
            add_table_row(start_row + g_cursor_row);
            if (g_table == gt::PTBL) ltable[start_row + g_cursor_row] = 0x88; // set pw
        }

        gui::disabled(g_cursor_row >= len);
        if (gui::button(gui::Icon::DeleteRow)) {
            delete_table_row(start_row + g_cursor_row);
            --len;
            --end_row;
            if (len == 0) {
                // delete jump row
                delete_table_row(start_row);
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
            app::slider(app::CANVAS_WIDTH, LABELS[i], adsr[i], 0, 0xf);
        }
        instr.ad = (adsr[0] << 4) | adsr[1];
        instr.sr = (adsr[2] << 4) | adsr[3];
    }
    else if (g_cursor_select == CursorSelect::GateTimer) {
        gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_HEIGHT });
        gui::text("GATE TIMER");

        gui::item_size({ app::CANVAS_WIDTH / 2, app::BUTTON_HEIGHT });
        gui::button_style(gui::ButtonStyle::Normal);
        if (gui::button("DISABLE HARD RESTART", instr.gatetimer & 0x80)) {
            instr.gatetimer ^= 0x80;
        }
        gui::same_line();
        if (gui::button("DISABLE GATE OFF", instr.gatetimer & 0x40)) {
            instr.gatetimer ^= 0x40;
        }
        int v = instr.gatetimer & 0x3f;
        app::slider(app::CANVAS_WIDTH, "%X", v, 1, 0xf, &instr.gatetimer);
        instr.gatetimer = (instr.gatetimer & 0xc0) | v;
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
            int mode = (lval <= 0x0f) ? 0 : (lval <= 0xef) ? 1 : 2;
            gui::item_size({ app::CANVAS_WIDTH / 3, app::BUTTON_HEIGHT });
            gui::button_style(gui::ButtonStyle::RadioLeft);
            if (gui::button("WAIT", mode == 0) && mode != 0) {
                mode = 0;
                lval = 0;
            }
            gui::same_line();
            gui::button_style(gui::ButtonStyle::RadioCenter);
            if (gui::button("WAVE", mode == 1) && mode != 1) {
                mode = 1;
                lval = 0x11;
            }
            gui::same_line();
            gui::button_style(gui::ButtonStyle::RadioRight);
            if (gui::button("COMMAND", mode == 2) && mode != 2) {
                mode = 2;
                lval = 0xf0;
                rval = 0x00;
            }

            gui::button_style(gui::ButtonStyle::Normal);
            if (mode == 0) {
                // wait
                int v = lval;
                app::slider(app::CANVAS_WIDTH, "  %X", v, 0, 0xf, &lval);
                lval = v;
            }
            else if (mode == 1) {
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
                    sprintf(str, "%c%02X", "+-"[v < 0], abs(v));
                    app::slider(app::CANVAS_WIDTH, str, v, -0x20, 0x20, &rval);
                    rval = v >= 0 ? v : v + 0x80;
                }
                else if (mode == 1) {
                    int v = rval - 0x80;
                    app::slider(app::CANVAS_WIDTH, " %02X", v, 1, 95, &rval);
                    rval = v + 0x80;
                }
            }
            else {
                // command
                gui::align(gui::Align::Left);
                gui::text("TODO");
                gui::align(gui::Align::Center);
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
            if (gui::button("STEP PULSE WIDTH", mode == 1) && mode != 1) {
                mode = 1;
                lval = 0x20;
                rval = 0x40;
            }
            if (mode == 0) {
                // set pw
                int v = ((lval & 0xf) << 8) | rval;
                app::slider(app::CANVAS_WIDTH, "%03X", v, 0, 0xfff, &rval);
                lval = 0x80 | (v >> 8);
                rval = v & 0xff;
            }
            else {
                // step pw
                app::slider(app::CANVAS_WIDTH, " %02X", lval, 1, 0x7f);
                int v = int8_t(rval);
                sprintf(str, "%c%02X", "+-"[v < 0], abs(v));
                app::slider(app::CANVAS_WIDTH, str, v, -128, 127, &rval);
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
                lval = 0x80;
                rval = 0xF0;
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
            if (gui::button("STEP CUTOFF", mode == 2) && mode != 2) {
                mode = 2;
                lval = 0x01;
                rval = 0x00;
            }

            if (mode == 0) {
                // set params
                gui::button_style(gui::ButtonStyle::Normal);
                gui::item_size({ app::CANVAS_WIDTH / 3, app::BUTTON_HEIGHT });
                if (gui::button(gui::Icon::Lowpass, lval & 0x10)) lval ^= 0x10;
                gui::same_line();
                if (gui::button(gui::Icon::Bandpass, lval & 0x20)) lval ^= 0x20;
                gui::same_line();
                if (gui::button(gui::Icon::Highpass, lval & 0x40)) lval ^= 0x40;

                if (gui::button("VOICE 1", rval & 0x1)) rval ^= 0x1;
                gui::same_line();
                if (gui::button("VOICE 2", rval & 0x2)) rval ^= 0x2;
                gui::same_line();
                if (gui::button("VOICE 3", rval & 0x4)) rval ^= 0x4;

                int v = rval >> 4;
                app::slider(app::CANVAS_WIDTH, "RES %X", v, 0, 0xf, &rval);
                rval = (rval & 0x0f) | (v << 4);
            }
            else if (mode == 1) {
                // set cutoff
                app::slider(app::CANVAS_WIDTH, " %02X", rval, 0, 0xff);
            }
            else {
                // step cutoff
                app::slider(app::CANVAS_WIDTH, " %02X", lval, 1, 0x7f);
                int v = int8_t(rval);
                sprintf(str, "%c%02X", "+-"[v < 0], abs(v));
                app::slider(app::CANVAS_WIDTH, str, v, -0x20, 0x20, &rval);
                rval = v;
            }
        }
    }



    // table sharing window
    if (draw_share_window) {
        int space = app::canvas_height() - 12 - app::BUTTON_HEIGHT * 2;
        int row_h = std::min<int>(space / 32, app::MAX_ROW_HEIGHT);
        enum {
            COL_W = 12 + 8 * 18,
        };
        gui::begin_window({ COL_W * 2, row_h * 32 + app::BUTTON_HEIGHT * 2 });
        gui::item_size({ COL_W * 2, app::BUTTON_HEIGHT });
        gui::text("%s TABLE SHARING", TABLE_LABELS[g_table]);

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
                draw_share_window = false;
                if (share_count == 1) {
                    // TODO: confirmation dialog
                    // delete table
                    for (int i = 0; i <= len; ++i) delete_table_row(start_row);
                }
                instr.ptr[g_table] = in.ptr[g_table];
            }
        }
        gui::align(gui::Align::Center);
        gui::button_style(gui::ButtonStyle::Normal);

        gui::item_size({ COL_W * 2 / 3, app::BUTTON_HEIGHT });
        gui::disabled(share_count < 2 || num_free_rows < (len + 1));
        if (gui::button("CLONE")) {
            draw_share_window = false;
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
            // TODO: update pointers in pattern and wave table
        }
        gui::same_line();
        gui::disabled(instr.ptr[g_table] == 0);
        if (gui::button("DELETE")) {
            draw_share_window = false;
            if (share_count == 1) {
                // delete table
                for (int i = 0; i <= len; ++i) delete_table_row(start_row);
            }
            instr.ptr[g_table] = 0;
        }
        gui::disabled(false);
        gui::same_line();
        if (gui::button("CLOSE")) draw_share_window = false;
        gui::end_window();
    }
}


void draw_hard() {

    enum class CursorSelect {
        WaveTable,
        PulseTable,
        FilterTable,
        SpeedTable,

        Attack,
        Decay,
        Sustain,
        Release,
        VibDelay,
        GateTimer,
        FirstWave,
    };

    static CursorSelect g_cursor_select = CursorSelect::WaveTable;
    static int          g_cursor_row    = 0;
    static int          g_table_scroll[4];

    int             instr_nr = piano::instrument();
    gt::Instrument& instr    = g_song.instruments[instr_nr];
    settings_view::Settings const& settings = settings_view::settings();
    char str[32];

    gui::same_line();
    gui::align(gui::Align::Left);
    gui::item_size({ 16 * 8 + 12, app::BUTTON_HEIGHT });
    gui::input_text(instr.name);
    gui::align(gui::Align::Center);

    gui::button_style(gui::ButtonStyle::TableCell);
    gui::item_size(app::BUTTON_HEIGHT);
    uint8_t adsr[] = {
        uint8_t(instr.ad >> 4),
        uint8_t(instr.ad & 0xf),
        uint8_t(instr.sr >> 4),
        uint8_t(instr.sr & 0xf),
    };
    for (int i = 0; i < 4; ++i) {
        sprintf(str, "%X", adsr[i]);
        gui::same_line();
        CursorSelect x = CursorSelect(i + int(CursorSelect::Attack));
        if (gui::button(str, g_cursor_select == x)) {
            g_cursor_select = x;
        }
    }
    uint8_t data[] = {
        instr.vibdelay,
        instr.gatetimer,
        instr.firstwave,
    };
    gui::item_size({ app::BUTTON_HEIGHT + 8, app::BUTTON_HEIGHT });
    for (int i = 0; i < 3; ++i) {
        sprintf(str, "%02X", data[i]);
        gui::same_line();
        CursorSelect x = CursorSelect(i + int(CursorSelect::VibDelay));
        if (gui::button(str, g_cursor_select == x)) {
            g_cursor_select = x;
        }
    }
    gui::button_style(gui::ButtonStyle::Normal);

    gui::DrawContext& dc = gui::draw_context();
    ivec2 cursor = gui::cursor();
    int table_page = (app::canvas_height() - cursor.y - piano::HEIGHT - app::BUTTON_HEIGHT * 3) / settings.row_height;
    int text_offset = (settings.row_height - 7) / 2;

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

            gui::item_size({ 28, settings.row_height });
            gui::Box num_box = gui::item_box();
            gui::same_line();
            gui::item_size({ 44, settings.row_height });
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
            dc.text(num_box.pos + ivec2(5, text_offset), str);
            dc.rgb(color::BACKGROUND_ROW);
            dc.fill(box);


            if (state == gui::ButtonState::Released) {
                g_cursor_select = CursorSelect(t);
                g_cursor_row    = row;
            }
            if (state != gui::ButtonState::Normal) {
                dc.rgb(color::BUTTON_HELD);
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
                        1, 1, 0,
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
            auto p = box.pos + ivec2(5, text_offset);
            dc.text(p, str);
            p.x += 16;
            sprintf(str, "%02X", rval);
            dc.rgb(colors[1]);
            dc.text(p, str);
        }

        gui::cursor({ 90 * t + 72, cursor.y });
        gui::item_size({ 18, settings.row_height * table_page + app::BUTTON_HEIGHT });
        gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
        gui::vertical_drag_bar(scroll, 0, gt::MAX_TABLELEN - table_page, table_page);
    }

    gui::cursor({ 0, cursor.y + settings.row_height * table_page + app::BUTTON_HEIGHT });

    switch (g_cursor_select) {
    case CursorSelect::Attack:
    case CursorSelect::Decay:
    case CursorSelect::Sustain:
    case CursorSelect::Release: {
        int i = int(g_cursor_select) - int(CursorSelect::Attack);
        constexpr char const* labels[] = {
            "ATTACK",
            "DECAY",
            "SUSTAIN",
            "RELEASE",
        };
        int v = adsr[i];
        gui::item_size({ 12 + 8 * 8, app::BUTTON_HEIGHT });
        gui::text(labels[i]);
        gui::item_size(app::BUTTON_HEIGHT);
        gui::same_line();
        if (gui::button(gui::Icon::Left) && v > 0) --v;
        gui::same_line();
        if (gui::button(gui::Icon::Right) && v < 0xf) ++v;
        gui::same_line();
        gui::item_size({ app::CANVAS_WIDTH - gui::cursor().x, app::BUTTON_HEIGHT });
        gui::drag_bar_style(gui::DragBarStyle::Normal);
        gui::horizontal_drag_bar(v, 0, 0xf, 1);
        adsr[i] = v;
        instr.ad = (adsr[0] << 4) | adsr[1];
        instr.sr = (adsr[2] << 4) | adsr[3];
        break;
    }

    case CursorSelect::VibDelay:
    case CursorSelect::GateTimer: {
        char const* label = g_cursor_select == CursorSelect::VibDelay ? "VIBDELAY" : "HR TIMER";
        uint8_t&    v     = g_cursor_select == CursorSelect::VibDelay ? instr.vibdelay : instr.gatetimer;
        gui::item_size({ 12 + 8 * 8, app::BUTTON_HEIGHT });
        gui::text(label);
        gui::item_size(app::BUTTON_HEIGHT);
        gui::same_line();
        if (gui::button(gui::Icon::Left) && v > 0) --v;
        gui::same_line();
        if (gui::button(gui::Icon::Right) && v < 0xff) ++v;
        gui::same_line();
        gui::item_size({ app::CANVAS_WIDTH - gui::cursor().x, app::BUTTON_HEIGHT });
        gui::drag_bar_style(gui::DragBarStyle::Normal);
        gui::horizontal_drag_bar(v, 0, 0xff, 1);
        break;
    }

    case CursorSelect::FirstWave: {
        gui::item_size({ 12 + 8 * 8, app::BUTTON_HEIGHT });
        gui::text("1ST WAVE");
        uint8_t& v = instr.firstwave;
        gui::item_size(app::BUTTON_HEIGHT);
        gui::same_line();
        if (gui::button(gui::Icon::Noise, v & 0x80)) v ^= 0x80;
        gui::same_line();
        if (gui::button(gui::Icon::Pulse, v & 0x40)) v ^= 0x40;
        gui::same_line();
        if (gui::button(gui::Icon::Saw, v & 0x20)) v ^= 0x20;
        gui::same_line();
        if (gui::button(gui::Icon::Triangle, v & 0x10)) v ^= 0x10;
        gui::same_line();
        if (gui::button(gui::Icon::Test, v & 0x08)) v ^= 0x08;
        gui::same_line();
        if (gui::button(gui::Icon::Ring, v & 0x04)) v ^= 0x04;
        gui::same_line();
        if (gui::button(gui::Icon::Sync, v & 0x02)) v ^= 0x02;
        gui::same_line();
        if (gui::button(gui::Icon::Gate, v & 0x01)) v ^= 0x01;
        break;
    }

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



void draw() {

    gui::item_size(app::BUTTON_HEIGHT);
    gui::align(gui::Align::Center);
    if (gui::button("E", g_easy_mode)) g_easy_mode ^= 1;
    gui::align(gui::Align::Left);

    if (g_easy_mode) draw_easy();
    else draw_hard();


    piano::draw();
}


} // namespace instrument_view
