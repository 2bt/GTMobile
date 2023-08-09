#include "instrument_view.hpp"
#include "piano.hpp"
#include "app.hpp"
#include "gui.hpp"


namespace instrument_view {
namespace {


int g_row_height = 13; // 9-16, default: 13

int g_table_scroll[4];

int g_cursor_table = 0;
int g_cursor_row   = 0;

using TagTable = std::array<bool, gt::MAX_TABLELEN>;
std::array<TagTable, gt::MAX_TABLES> g_used_in_patterns;


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


void init() {
    gt::Song const& song = app::song();

    g_used_in_patterns = {};
    for (int p = 0; p < gt::MAX_PATT; ++p) {
        auto& pattern = song.pattern[p];
        int   len     = song.pattlen[p];

        for (int i = 0; i < len; ++i) {
            uint8_t cmd = pattern[i * 4 + 2];
            uint8_t val = pattern[i * 4 + 3];
            if (val == 0) continue;
            switch (cmd) {
            case gt::CMD_SETWAVEPTR:
                tag_used(gt::WTBL, g_used_in_patterns[gt::WTBL], val - 1);
                break;
            case gt::CMD_SETPULSEPTR:
                tag_used(gt::PTBL, g_used_in_patterns[gt::PTBL], val - 1);
                break;
            case gt::CMD_SETFILTERPTR:
                tag_used(gt::FTBL, g_used_in_patterns[gt::FTBL], val - 1);
                break;
            case gt::CMD_PORTAUP:
            case gt::CMD_PORTADOWN:
            case gt::CMD_TONEPORTA:
            case gt::CMD_VIBRATO:
            case gt::CMD_FUNKTEMPO:
                tag_used(gt::STBL, g_used_in_patterns[gt::STBL], val - 1);
                break;
            default: break;
            }
        }
    }
}


void draw() {

    int        instr_nr = piano::instrument();
    gt::Song&  song     = app::song();
    gt::Instr& instr    = song.instr[instr_nr];

    gui::align(gui::Align::Left);
    gui::item_size({ 2 * 8 + 12, app::BUTTON_WIDTH });

    char str[32];
    sprintf(str, "%02X", instr_nr);
    if (gui::button(str)) piano::show_instrument_select();

    gui::same_line();
    gui::item_size({ 16 * 8 + 12, app::BUTTON_WIDTH });
    gui::input_text(instr.name);



    gui::same_line();
    gui::item_size(app::BUTTON_WIDTH);
    sprintf(str, "%X", instr.ad >> 4);
    gui::button(str);
    sprintf(str, "%X", instr.ad & 0xf);
    gui::same_line();
    gui::button(str);

    sprintf(str, "%X", instr.sr >> 4);
    gui::same_line();
    gui::button(str);
    sprintf(str, "%X", instr.sr & 0xf);
    gui::same_line();
    gui::button(str);



    // TODO
    // + wave ptr
    // + pulse ptr
    // + filter ptr
    // + vibrato ptr
    // + vibrato delay
    // + gate timer
    // + first wave

    gui::DrawContext& dc = gui::draw_context();

    ivec2 cursor = gui::cursor();
    int table_page = (app::canvas_height() - cursor.y - 68) / g_row_height;
    int text_offset = (g_row_height - 7) / 2;

    for (int t = 0; t < 4; ++t) {
        auto& ltable = song.ltable[t];
        auto& rtable = song.rtable[t];

        TagTable used = {};
        int pos = instr.ptr[t] - 1;
        tag_used(t, used, pos);

        gui::cursor({ 90 * t, cursor.y });
        gui::item_size({ 72, g_row_height });

        int& scroll = g_table_scroll[t];
        for (int i = 0; i < table_page; ++i) {
            int row = scroll + i;
            uint8_t& lval = ltable[row];
            uint8_t& rval = rtable[row];

            gui::Box box = gui::item_box();

            dc.color(color::ROW_NUMBER);
            if (row == pos) {
                dc.color(color::BUTTON_ACTIVE);
                dc.fill({ box.pos + ivec2(1, 0), ivec2(26, g_row_height) });
                dc.color(color::WHITE);
            }
            // row number
            ivec2 p = box.pos + ivec2(6, text_offset);
            sprintf(str, "%02X", row + 1);
            dc.text(p, str);
            p.x += 28;

            dc.color(used[row] ? color::HIGHLIGHT_ROW : color::BACKGROUND_ROW);
            dc.fill({ box.pos + ivec2(29, 0), ivec2(42, g_row_height) });



            gui::ButtonState state = gui::button_state(box);
            if (state == gui::ButtonState::Released) {
                g_cursor_table = t;
                g_cursor_row   = row;
            }
            if (state != gui::ButtonState::Normal) {
                dc.color(color::BUTTON_HELD);
                dc.box(box, gui::BoxStyle::Cursor);
            }
            if (t == g_cursor_table && row == g_cursor_row) {
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
        gui::item_size({ 18, g_row_height * table_page });
        gui::vertical_drag_bar(scroll, 0, gt::MAX_TABLELEN - table_page, table_page);

    }


    piano::draw();
}


} // namespace instrument_view