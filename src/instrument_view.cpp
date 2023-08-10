#include "instrument_view.hpp"
#include "piano.hpp"
#include "app.hpp"
#include "gui.hpp"
#include <cassert>


namespace instrument_view {
namespace {

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


CursorSelect g_cursor_select = CursorSelect::WaveTable;
int          g_cursor_row    = 0;
int          g_table_scroll[4];


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


} // namespace


void draw() {

    int        instr_nr = piano::instrument();
    gt::Song&  song     = app::song();
    gt::Instr& instr    = song.instr[instr_nr];
    app::Settings const& settings = app::settings();

    gui::align(gui::Align::Left);
    gui::item_size({ 2 * 8 + 12, app::BUTTON_WIDTH });

    char str[32];
    sprintf(str, "%02X", instr_nr);
    if (gui::button(str)) piano::show_instrument_select();

    gui::same_line();
    gui::item_size({ 16 * 8 + 12, app::BUTTON_WIDTH });
    gui::input_text(instr.name);


    gui::button_style(gui::ButtonStyle::TableCell);
    gui::item_size(app::BUTTON_WIDTH);
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
    gui::item_size({ app::BUTTON_WIDTH + 8, app::BUTTON_WIDTH });
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
    int table_page = (app::canvas_height() - cursor.y - 68 - app::BUTTON_WIDTH * 3) / settings.row_height;
    int text_offset = (settings.row_height - 7) / 2;

    for (int t = 0; t < 4; ++t) {
        gui::cursor({ 90 * t, cursor.y });

        // table pointers
        gui::item_size({ 72, app::BUTTON_WIDTH });
        sprintf(str, "%02X", instr.ptr[t]);
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

        gui::item_size({ 72, settings.row_height });

        auto& ltable = song.ltable[t];
        auto& rtable = song.rtable[t];

        int& scroll = g_table_scroll[t];
        for (int i = 0; i < table_page; ++i) {
            int row = scroll + i;
            uint8_t& lval = ltable[row];
            uint8_t& rval = rtable[row];

            gui::Box box = gui::item_box();
            gui::ButtonState state = gui::button_state(box);
            box.pos.x += 1;
            box.size.x -= 2;

            dc.color(color::ROW_NUMBER);
            if (row == pos) {
                dc.color(color::BUTTON_ACTIVE);
                dc.fill({ box.pos, ivec2(26, settings.row_height) });
                dc.color(color::WHITE);
            }
            // row number
            ivec2 p = box.pos + ivec2(5, text_offset);
            sprintf(str, "%02X", row + 1);
            dc.text(p, str);
            p.x += 28;

            dc.color(used[row] ? color::HIGHLIGHT_ROW : color::BACKGROUND_ROW);
            dc.fill({ box.pos + ivec2(28, 0), ivec2(42, settings.row_height) });


            if (state == gui::ButtonState::Released) {
                g_cursor_select = CursorSelect(t);
                g_cursor_row    = row;
            }
            if (state != gui::ButtonState::Normal) {
                dc.color(color::BUTTON_HELD);
                dc.box(box, gui::BoxStyle::Cursor);
            }
            if (CursorSelect(t) == g_cursor_select && row == g_cursor_row) {
                dc.color(color::BUTTON_ACTIVE);
                dc.box(box, gui::BoxStyle::Cursor);
            }

            // chose colors
            u8vec4 colors[2] = {
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


            // fade colors
            if (!used[row]) {
                // colors[0] = mix(colors[0], color::BLACK, 0.3f);
                // colors[1] = mix(colors[1], color::BLACK, 0.3f);
            }

            if (lval == 0xff) {
                sprintf(str, "\x06\x07"); // goto arrow
            }
            else {
                sprintf(str, "%02X", lval);
            }
            dc.color(colors[0]);
            dc.text(p, str);
            p.x += 16;
            sprintf(str, "%02X", rval);
            dc.color(colors[1]);
            dc.text(p, str);
        }

        gui::cursor({ 90 * t + 72, cursor.y });
        gui::item_size({ 18, settings.row_height * table_page + app::BUTTON_WIDTH });
        gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
        gui::vertical_drag_bar(scroll, 0, gt::MAX_TABLELEN - table_page, table_page);
    }

    gui::cursor({ 0, cursor.y + settings.row_height * table_page + app::BUTTON_WIDTH });

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
        gui::item_size({ 12 + 8 * 8, app::BUTTON_WIDTH });
        gui::text(labels[i]);
        gui::item_size(app::BUTTON_WIDTH);
        gui::same_line();
        if (gui::button(gui::Icon::Left) && v > 0) --v;
        gui::same_line();
        if (gui::button(gui::Icon::Right) && v < 0xf) ++v;
        gui::same_line();
        gui::item_size({ app::CANVAS_WIDTH - gui::cursor().x, app::BUTTON_WIDTH });
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
        gui::item_size({ 12 + 8 * 8, app::BUTTON_WIDTH });
        gui::text(label);
        gui::item_size(app::BUTTON_WIDTH);
        gui::same_line();
        if (gui::button(gui::Icon::Left) && v > 0) --v;
        gui::same_line();
        if (gui::button(gui::Icon::Right) && v < 0xff) ++v;
        gui::same_line();
        gui::item_size({ app::CANVAS_WIDTH - gui::cursor().x, app::BUTTON_WIDTH });
        gui::drag_bar_style(gui::DragBarStyle::Normal);
        gui::horizontal_drag_bar(v, 0, 0xff, 1);
        break;
    }

    case CursorSelect::FirstWave: {
        gui::item_size({ 12 + 8 * 8, app::BUTTON_WIDTH });
        gui::text("1ST WAVE");
        uint8_t& v = instr.firstwave;
        gui::item_size({ 35, app::BUTTON_WIDTH });
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
    }

    case CursorSelect::WaveTable:
    case CursorSelect::PulseTable:
    case CursorSelect::FilterTable:
    case CursorSelect::SpeedTable: {
        gui::align(gui::Align::Center);
        int t = int(g_cursor_select);
        uint8_t* data[] = {
            &song.ltable[t][g_cursor_row],
            &song.rtable[t][g_cursor_row],
        };

        for (int n = 0; n < 2; ++ n) {
            for (int i = 0; i < 16; ++i) {
                gui::item_size({ 22 + (i % 2), app::BUTTON_WIDTH });
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
        gui::item_size({ app::CANVAS_WIDTH, app::BUTTON_WIDTH });
        gui::button("FOOBAR");
        break;
    }


    piano::draw();
}


} // namespace instrument_view