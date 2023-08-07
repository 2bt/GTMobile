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

} // namespace


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

        bool used[gt::MAX_TABLELEN] = {};
        int pos = instr.ptr[t] - 1;
        while (pos >= 0 && pos < gt::MAX_TABLELEN && !used[pos]) {
            used[pos] = true;
            if (t == 3) break;
            if (ltable[pos] == 0xff) {
                pos = rtable[pos] - 1;
            }
            else ++pos;
        }

        gui::cursor({ 90 * t, cursor.y });
        gui::item_size({ 70, g_row_height });

        int& scroll = g_table_scroll[t];
        for (int i = 0; i < table_page; ++i) {
            int row = scroll + i;
            uint8_t& lval = ltable[row];
            uint8_t& rval = rtable[row];

            gui::Box box = gui::item_box();

            ivec2 p = box.pos + ivec2(6, text_offset);
            dc.color(color::ROW_NUMBER);
            sprintf(str, "%02X", row + 1);
            dc.text(p, str);
            p.x += 24;

            box.pos.x += 26;
            box.size.x -= 26;
            dc.color(color::BACKGROUND_ROW);
            if (used[row]) {
                dc.color(color::HIGHLIGHT_ROW);
            }
            dc.fill({ box.pos + ivec2(1, 0), box.size - ivec2(2, 0) });


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

                if (lval >= 0x10 && lval <= 0xDE) {
                    colors[0] = color::CMDS[7]; // color like waveform command
                }


                if (lval >= 0xf0 && lval <= 0xfe) {
                    // pattern command
                    constexpr bool valid_command[] = {
                        0, 1, 1, 1,
                        1, 1, 1, 1,
                        0, 1, 1, 1,
                        1, 1, 0,
                    };
                    int cmd = lval & 0xf;
                    if (valid_command[cmd]) {
                        colors[0] = color::CMDS[cmd];
                        colors[1] = color::CMDS[cmd];
                    }
                    else {
                        colors[0] = color::DARK_GREY;
                        colors[1] = color::DARK_GREY;
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

            // fade colors
            if (!used[row]) {
                colors[0] = mix(colors[0], color::BLACK, 0.4f);
                colors[1] = mix(colors[1], color::BLACK, 0.4f);
            }

            if (lval == 0xff) {
                sprintf(str, "\x06\x07"); // goto arrow
            }
            else {
                sprintf(str, "%02X", lval);
            }
            dc.color(colors[0]);
            dc.text(p, str);
            p.x += 20;
            sprintf(str, "%02X", rval);
            dc.color(colors[1]);
            dc.text(p, str);

        }

        gui::cursor({ 90 * t + 70, cursor.y });
        gui::item_size({ app::BUTTON_WIDTH, g_row_height * table_page });
        gui::vertical_drag_bar(scroll, 0, gt::MAX_TABLELEN - table_page, table_page);

    }


    piano::draw();
}


} // namespace instrument_view