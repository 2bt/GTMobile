#include "instrument_view.hpp"
#include "settings_view.hpp"
#include "piano.hpp"
#include "app.hpp"
#include "gui.hpp"
#include <cassert>


namespace instrument_view {
namespace {

bool         g_easy_mode = true;

using TagTable = std::array<bool, gt::MAX_TABLELEN>;


void tag_used(int table, TagTable& tag, int pos) {
    gt::Song const& song = app::song();
    auto& ltable = song.ltable[table];
    auto& rtable = song.rtable[table];
    while (pos >= 0 && pos < gt::MAX_TABLELEN && !tag[pos]) {
        tag[pos] = true;
        if (table == gt::STBL) break;
        if (ltable[pos] == 0xff) {
            pos = rtable[pos] - 1;
        }
        else ++pos;
    }
}


void draw_easy() {
    gt::Song&       song     = app::song();
    int             instr_nr = piano::instrument();
    gt::Instrument& instr    = song.instruments[instr_nr];
    char str[32];

    enum class CursorSelect {
        Adsr,
        //...
        Table,
    };
    static CursorSelect g_cursor_select = CursorSelect::Adsr;

    gui::same_line();
    gui::align(gui::Align::Left);
    gui::item_size({ 16 * 8 + 12, app::BUTTON_HEIGHT });
    gui::input_text(instr.name);
    gui::align(gui::Align::Center);

    // adsr
    gui::item_size({ 12 + 4 * 8, app::BUTTON_HEIGHT });
    int adsr[] = {
        instr.ad >> 4,
        instr.ad & 0xf,
        instr.sr >> 4,
        instr.sr & 0xf,
    };
    gui::button_style(gui::ButtonStyle::TableCell);
    sprintf(str, "%X%X%X%X", adsr[0], adsr[1], adsr[2], adsr[3]);
    gui::same_line();
    if (gui::button(str, g_cursor_select == CursorSelect::Adsr)) {
        g_cursor_select = CursorSelect::Adsr;
    }

    // tables
    enum {
        CW_NUM = 28,
        CW_DATA = app::CANVAS_WIDTH - CW_NUM - app::SCROLL_WIDTH - app::BUTTON_HEIGHT * 2,
        PANEL_W = app::CANVAS_WIDTH - 10,
        LABEL_W = 12 + 8*3,
        SLIDER_W = PANEL_W - LABEL_W,
    };

    static int g_table      = 0;
    static int g_cursor_row = 0;
    static int g_scroll     = 0;

    gui::item_size({ app::CANVAS_WIDTH / 3, app::BUTTON_HEIGHT });
    for (int t = 0; t < 3; ++t) {
        constexpr char const* LABELS[] = { "WAVE", "PULSE", "FILTER" };

        gui::button_style(instr.ptr[t] > 0 ? gui::ButtonStyle::Tab : gui::ButtonStyle::ShadedTab );
        if (gui::button(LABELS[t], t == g_table)) {
            g_table = t;
        }
        gui::same_line();
    }
    gui::button_style(gui::ButtonStyle::Normal);
    gui::same_line(false);

    ivec2 cursor      = gui::cursor();
    int   table_space = app::canvas_height() - cursor.y - piano::HEIGHT - app::BUTTON_HEIGHT * 4 - 10;
    int   table_page  = table_space / app::MAX_ROW_HEIGHT;
    int   max_scroll  = 0;
    int   text_offset = (app::MAX_ROW_HEIGHT - 7) / 2;


    auto& ltable = song.ltable[g_table];
    auto& rtable = song.rtable[g_table];
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
        max_scroll = std::max(max_scroll, len);
    }
    g_cursor_row = std::min(g_cursor_row, len - 1);
    g_cursor_row = std::max(g_cursor_row, 0);

    gui::DrawContext& dc = gui::draw_context();
    for (int i = 0; i < table_page; ++i) {
        int r = i + g_scroll;
        sprintf(str, "%02X", r + 1);
        gui::item_size({ CW_NUM, app::MAX_ROW_HEIGHT });
        gui::Box box = gui::item_box();
        dc.rgb(color::ROW_NUMBER);
        dc.text(box.pos + ivec2(6, text_offset), str);
        gui::same_line();

        gui::item_size({ CW_DATA, app::MAX_ROW_HEIGHT });
        box = gui::item_box();
        if (r >= len) continue;

        gui::ButtonState state = gui::button_state(box);
        box.pos.x += 1;
        box.size.x -= 2;

        dc.rgb(color::BACKGROUND_ROW);
        dc.fill(box);

        // loop marker
        if (start_row + r + 1 == rtable[end_row]) {
            dc.rgb(color::BUTTON_HELD);
            dc.text(box.pos + ivec2(box.size.x - 8, text_offset), "\x05");
        }

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
    int y = gui::cursor().y;

    // scrolling
    gui::cursor({ CW_NUM + CW_DATA, cursor.y });
    gui::item_size({ app::SCROLL_WIDTH, app::MAX_ROW_HEIGHT * table_page });
    gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
    gui::vertical_drag_bar(g_scroll, 0, max_scroll - table_page, table_page);



    int free_row = ltable.size();
    while (free_row > 0 && ltable[free_row - 1] == 0 && rtable[free_row - 1] == 0) {
        --free_row;
    }
    int num_free_rows = ltable.size() - free_row;
    // we need two free rows to add a single row to an emtpy instr table
    bool can_add_row = len == 0 ? num_free_rows >= 2 : num_free_rows > 0;

    gui::cursor({ app::CANVAS_WIDTH - app::BUTTON_HEIGHT * 2, cursor.y });
    gui::item_size(app::BUTTON_HEIGHT);
    // gui::disabled(!can_add_row);
    if (gui::button(gui::Icon::AddRowAbove)) {
        // TODO
    }
    gui::same_line();
    // gui::disabled(!(len < MAX_SONG_ROWS  && pos < len));
    if (gui::button(gui::Icon::AddRowBelow)) {
        // TODO
    }

    gui::disabled(g_cursor_row >= len);
    if (gui::button(gui::Icon::DeleteRow)) {
        // TODO
    }
    gui::same_line();
    bool is_loop = rtable[end_row] == start_row + g_cursor_row + 1;
    if (gui::button(gui::Icon::JumpBack)) {
        if (is_loop) rtable[end_row] = 0;
        else rtable[end_row] = start_row + g_cursor_row + 1;
    }
    gui::disabled(false);

    if (g_cursor_row >= len) return;

    // draw frame
    dc.rgb(color::BUTTON_NORMAL);
    dc.box({ { -1, y - 1 }, { app::CANVAS_WIDTH + 2, 12 + app::BUTTON_HEIGHT * 4 } }, gui::BoxStyle::Window);
    gui::cursor({ 5, y + 5 });
    const ivec2 LABEL_SIZE = { LABEL_W, app::BUTTON_HEIGHT };


    if (g_cursor_select == CursorSelect::Adsr) {
        for (int i = 0; i < 4; ++i) {
            constexpr char const* LABELS[] = {
                "A %X",
                "D %X",
                "S %X",
                "R %X",
            };
            gui::item_size(LABEL_SIZE);
            gui::text(LABELS[i], adsr[i]);
            gui::same_line();
            app::slider(SLIDER_W, adsr[i], 0, 0xf);
        }
        instr.ad = (adsr[0] << 4) | adsr[1];
        instr.sr = (adsr[2] << 4) | adsr[3];
    }
    else if (g_cursor_select == CursorSelect::Table) {
        uint8_t& lval = ltable[start_row + g_cursor_row];
        uint8_t& rval = rtable[start_row + g_cursor_row];
        if (g_table == gt::WTBL) {
            // left side:
            // + 00-0F delay
            // + 10-DF waveform
            // + E0-EF inaudible waveform 00-0F
            // + F0-FE pattern command
            // + FF    jump
            int mode;
            if (lval <= 0xf)       mode = 0;
            else if (lval <= 0xef) mode = 1;
            else                   mode = 2;
            gui::item_size({ PANEL_W / 3 + 1, app::BUTTON_HEIGHT });
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
            gui::item_size({ PANEL_W / 3, app::BUTTON_HEIGHT });
            gui::button_style(gui::ButtonStyle::RadioRight);
            if (gui::button("COMMAND", mode == 2) && mode != 2) {
                mode = 2;
                // TODO
                lval = 0xf0;
                rval = 0x00;
            }

            gui::button_style(gui::ButtonStyle::Normal);
            if (mode == 0) {
                // wait
                int v = lval;
                gui::item_size(LABEL_SIZE);
                gui::text("  %X", v);
                gui::same_line();
                app::slider(SLIDER_W, v, 0, 0xf, &lval);
                lval = v;
            }
            else if (mode == 1) {
                // wave
                uint8_t v = lval;
                if (v >= 0xe0) v -= 0xe0;
                for (int i = 0; i < 8; ++i) {
                    gui::item_size({ PANEL_W / 8 + (i % 4 < 3), app::BUTTON_HEIGHT });
                    int mask = 0x80 >> i;
                    int icon = int(gui::Icon::Noise) + i;
                    if (gui::button(gui::Icon(icon), v & mask)) v ^= mask;
                    gui::same_line();
                }
                gui::same_line(false);
                if (v > 0xdf) v ^= 0x80;
                if (v < 0x10) v += 0xe0;
                lval = v;
            }
            if (mode != 2) {
                // right side:
                // + 00-5F relative note
                // + 60-7F negative relative note
                // + 80    do nothing
                // + 81-DD absolute notes C#0 - B-7
                if (rval < 0x80) mode = 0;
                else if (rval > 0x80) mode = 1;
                else mode = 2;
                gui::item_size({ PANEL_W / 3 + 1, app::BUTTON_HEIGHT });
                gui::button_style(gui::ButtonStyle::RadioLeft);
                if (gui::button("RELATIVE", mode == 0) && mode != 0) {
                    mode = 0;
                    rval = 0;
                }
                gui::same_line();
                gui::button_style(gui::ButtonStyle::RadioCenter);
                if (gui::button("ABSOLUTE", mode == 1) && mode != 1) {
                    mode = 1;
                    rval = 0x80 + 48;
                }
                gui::same_line();
                gui::item_size({ PANEL_W / 3, app::BUTTON_HEIGHT });
                gui::button_style(gui::ButtonStyle::RadioRight);
                if (gui::button("NO CHANGE", mode == 2) && mode != 2) {
                    mode = 2;
                    rval = 0x80;
                }

                if (mode == 0) {
                    int v = rval < 0x60 ? rval : rval - 0x80;
                    gui::item_size(LABEL_SIZE);
                    gui::text("%c%02X", "+-"[v < 0], abs(v));
                    gui::same_line();
                    // NOTE: don't show full range
                    app::slider(SLIDER_W, v, -0x20, 0x20, &rval);
                    rval = v >= 0 ? v : v + 0x80;
                }
                else if (mode == 1) {
                    int v = rval - 0x80;
                    gui::item_size(LABEL_SIZE);
                    gui::text(" %02X", v);
                    gui::same_line();
                    app::slider(SLIDER_W, v, 1, 95, &rval);
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
            int mode = lval >= 0x80;
            gui::item_size({ PANEL_W / 2, app::BUTTON_HEIGHT });
            gui::button_style(gui::ButtonStyle::RadioLeft);
            if (gui::button("STEP", mode == 0) && mode != 0) {
                mode = 0;
                lval = 0x20;
                rval = 0x40;
            }
            gui::same_line();
            gui::button_style(gui::ButtonStyle::RadioRight);
            if (gui::button("SET", mode == 1) && mode != 1) {
                mode = 1;
                lval = 0x88;
                rval = 0x00;
            }
            if (mode == 0) {
                gui::item_size(LABEL_SIZE);
                gui::text(" %02X", lval);
                gui::same_line();
                app::slider(SLIDER_W, lval, 1, 0x7f);
                int v = int8_t(rval);
                gui::item_size(LABEL_SIZE);
                gui::text("%c%02X", "+-"[v < 0], abs(v));
                gui::same_line();
                app::slider(SLIDER_W, v, -128, 127, &rval);
                rval = v;
            }
            else {
                int v = ((lval & 0xf) << 8) | rval;
                gui::item_size(LABEL_SIZE);
                gui::text("%03X", v);
                gui::same_line();
                app::slider(SLIDER_W, v, 0, 0xfff, &rval);
                lval = 0x80 | (v >> 8);
                rval = v & 0xff;
            }

        }
        else if (g_table == gt::FTBL) {
            gui::text("TODO");


        }
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
    gt::Song&       song     = app::song();
    gt::Instrument& instr    = song.instruments[instr_nr];
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

        TagTable used = {};
        int pos = instr.ptr[t] - 1;
        tag_used(t, used, pos);


        auto& ltable = song.ltable[t];
        auto& rtable = song.rtable[t];

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
            dc.rgb(used[row] ? color::HIGHLIGHT_ROW : color::BACKGROUND_ROW);
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
            &song.ltable[t][g_cursor_row],
            &song.rtable[t][g_cursor_row],
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
