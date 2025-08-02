#include "command_edit.hpp"
#include "piano.hpp"

namespace command_edit {
namespace {

gt::Song&               g_song = app::song();
bool                    g_edit_enabled;
Location                g_location;
uint8_t                 g_command;
std::array<uint8_t, 16> g_command_data;
CommandCallback         g_callback;
std::array<int, 3>      g_table_scrolls;
int                     g_ptr_scroll;


} // namespace

void reset() {
    g_edit_enabled = false;
    g_command_data = {
        0,
        0, 0, 0,
        0,
        0, 0,
        0, 0, 0, 0, 0, 0,
        0x0f, // master volume
        0,
        0x06, // tempo
    };
    g_table_scrolls = {};
    g_ptr_scroll = 0;
}

void init(Location location, uint8_t cmd, uint8_t data, std::function<void(uint8_t, uint8_t)> cb) {
    g_edit_enabled      = true;
    g_location          = location;
    g_command           = cmd;
    g_command_data[cmd] = data;
    g_callback          = std::move(cb);
}


void draw() {
    if (!g_edit_enabled) return;

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
    gui::text("COMMAND");
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
        g_edit_enabled = false;
    }

    uint8_t prev_command = g_command;
    uint8_t prev_data    = g_command_data[g_command];

    gui::cursor(cmd_cursor);
    gui::align(gui::Align::Left);
    gui::item_size({ C1, app::MAX_ROW_HEIGHT });
    for (int i = 0; i < 16; ++i) {
        if (g_location == Location::WaveTable) {
            if (i == gt::CMD_DONOTHING || i == gt::CMD_SETWAVEPTR || i >= gt::CMD_FUNKTEMPO) {
                gui::item_box();
                continue;
            }
        }
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
        if (gui::button("", i == g_command)) {
            g_command = i;
        }
        dc.rgb(color::WHITE);
        dc.text(c + ivec2(12 + 8 * 3 + 6 - 1, 6), CMD_LABELS[i]);

        gui::Box b = { c + ivec2(1, 1), ivec2(36 - 1, app::MAX_ROW_HEIGHT - 2) };
        dc.rgb(color::BACKGROUND_ROW);
        dc.fill(b);
        uint8_t data = g_command_data[i];
        if (data > 0 && i == gt::CMD_VIBRATO)   data -= gt::STBL_VIB_START;
        if (data > 0 && i == gt::CMD_FUNKTEMPO) data -= gt::STBL_FUNK_START;
        sprintf(str, "%X%02X", i, data);
        gui::disabled(g_command != i);
        dc.rgb(color::CMDS[i]);
        dc.text(b.pos + 5, str);
        gui::disabled(false);
    }
    gui::align(gui::Align::Center);
    gui::cursor(button_cursor);

    auto& data = g_command_data[g_command];

    // handle all command that reference the speed table
    if (g_command == gt::CMD_PORTAUP ||
        g_command == gt::CMD_PORTADOWN ||
        g_command == gt::CMD_TONEPORTA ||
        g_command == gt::CMD_VIBRATO ||
        g_command == gt::CMD_FUNKTEMPO)
    {
        enum CmdType { CMD_PORTA, CMD_VIB, CMD_FUNK };
        CmdType cmd_type = CMD_PORTA;
        if (g_command == gt::CMD_VIBRATO) cmd_type = CMD_VIB;
        if (g_command == gt::CMD_FUNKTEMPO) cmd_type = CMD_FUNK;

        gui::cursor(table_cursor);

        auto& ltable = g_song.ltable[gt::STBL];
        auto& rtable = g_song.rtable[gt::STBL];

        gui::align(gui::Align::Left);
        for (int i = 0; i < PAGE; ++i) {
            int r = i + g_table_scrolls[cmd_type];
            char* s = str;
            s += sprintf(s, "%02X ", r);

            uint8_t lval = 0;
            uint8_t rval = 0;
            if (r > 0) {
                if (cmd_type == CMD_PORTA) r += gt::STBL_PORTA_START;
                if (cmd_type == CMD_VIB)   r += gt::STBL_VIB_START;
                if (cmd_type == CMD_FUNK)  r += gt::STBL_FUNK_START;
                lval = ltable[r - 1];
                rval = rtable[r - 1];
            }
            bool is_set = lval | rval;

            ivec2 p = gui::cursor() + ivec2(6);
            gui::item_size({ CC, app::MAX_ROW_HEIGHT });
            gui::button_style(is_set || r == 0 ? gui::ButtonStyle::Normal : gui::ButtonStyle::Shaded);
            if (r == 0) {
                if (g_command == gt::CMD_TONEPORTA) sprintf(s, "TIE NOTE");
                else if (g_command == gt::CMD_FUNKTEMPO) sprintf(s, "NO CHANGE");
                else sprintf(s, "OFF");
            }
            if (is_set) {
                if (g_command <= gt::CMD_TONEPORTA) {
                    if (lval < 0x80) {
                        sprintf(s, "%02X %02X", lval, rval);
                    }
                    else {
                        sprintf(s, "   \x17%X", rval); // RT speed
                    }
                }
                else if (g_command == gt::CMD_VIBRATO) {
                    if (lval < 0x80) {
                        sprintf(s, "%02X %02X", lval, rval);
                    }
                    else {
                        sprintf(s, "%02X \x17%X", lval & 0x7f, rval); // RT speed
                    }
                }
                else {
                    sprintf(s, "%02X %02X", lval, rval);
                }
                dc.text(p, str);
            }
            if (gui::button(str, data == r)) data = r;
        }
        gui::align(gui::Align::Center);
        gui::button_style(gui::ButtonStyle::Normal);

        gui::cursor(table_cursor + ivec2(CC, 0));
        gui::item_size({ app::SCROLL_WIDTH, TABLE_HEIGHT });

        gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
        int max_scroll = std::max(0, gt::STBL_SUBTABLE_LEN - PAGE);
        gui::vertical_drag_bar(g_table_scrolls[cmd_type], 0, max_scroll, PAGE);

        gui::cursor(button_cursor);
        if (data > 0) {
            uint8_t& lval = ltable[data - 1];
            uint8_t& rval = rtable[data - 1];

            if (g_command <= gt::CMD_TONEPORTA) {

                gui::item_size({ WIDTH / 2, app::BUTTON_HEIGHT });
                gui::button_style(gui::ButtonStyle::RadioLeft);
                if (gui::button("PRECALCULATED", !(lval & 0x80)) && (lval & 0x80)) {
                    int note = piano::note();
                    uint16_t speed = gt::get_freq(note + 1) - gt::get_freq(note);
                    speed >>= rval;
                    lval = (speed >> 8) & 0x7f;
                    rval = speed & 0xff;
                }
                gui::same_line();
                gui::button_style(gui::ButtonStyle::RadioRight);
                if (gui::button("NOTE-INDEPENDENT", lval & 0x80) && !(lval & 0x80)) {
                    rval = 2;
                    lval = 0x80;
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
                int v = lval & 0x7f;
                gui::slider(WIDTH, "VIBRATO STEPS %02X", v, 0, 0x7f);
                lval = (lval & 0x80) | v;
                gui::item_size({ WIDTH / 2, app::BUTTON_HEIGHT });
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
                    gui::slider(WIDTH, "SHIFT  %X", rval, 0, 8);
                }
                else {
                    gui::slider(WIDTH, "SPEED %02X", rval, 0, 0xff);
                }
            }
            else if (g_command == gt::CMD_FUNKTEMPO) {
                gui::slider(WIDTH, "EVEN ROW %02X", lval, 0, 0xff);
                gui::slider(WIDTH, "ODD ROW  %02X", rval, 0, 0xff);
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
    else if (g_command == gt::CMD_SETWAVEPTR ||
             g_command == gt::CMD_SETPULSEPTR ||
             g_command == gt::CMD_SETFILTERPTR)
    {
        gui::cursor(table_cursor);
        gui::align(gui::Align::Left);
        gui::item_size({ CC, app::MAX_ROW_HEIGHT });
        for (int i = 0; i < PAGE; ++i) {
            int r = i + g_ptr_scroll;
            gt::Instrument const& instr = g_song.instruments[r];
            sprintf(str, "%02X %s", r, r == 0 ? "OFF" : instr.name.data());
            bool is_set = r == 0 || instr.ptr[g_command - gt::CMD_SETWAVEPTR] != 0;
            gui::button_style(is_set ? gui::ButtonStyle::Normal : gui::ButtonStyle::Shaded);
            if (gui::button(str, data == r)) data = r;
        }
        gui::button_style(gui::ButtonStyle::Normal);
        gui::disabled(false);
        gui::align(gui::Align::Center);

        gui::cursor(table_cursor + ivec2(CC, 0));
        gui::item_size({ app::SCROLL_WIDTH, TABLE_HEIGHT });
        gui::drag_bar_style(gui::DragBarStyle::Scrollbar);
        gui::vertical_drag_bar(g_ptr_scroll, 0, gt::MAX_INSTR - PAGE, PAGE);
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
        gui::item_size({ WIDTH / 2, app::BUTTON_HEIGHT });

        gui::button_style(gui::ButtonStyle::RadioLeft);
        if (gui::button("THIS VOICE ONLY", data & 0x80)) {
            data |= 0x80;
        }
        gui::same_line();
        gui::button_style(gui::ButtonStyle::RadioRight);
        if (gui::button("ALL VOICES", !(data & 0x80))) {
            data &= ~0x80;
        }

        int v = data & 0x7f;
        gui::slider(WIDTH, "%02X", v, 0, 0x7f);
        data = (data & 0x80) | v;
    }

    // apply changes
    if (g_command != prev_command || g_command_data[g_command] != prev_data) {
        g_callback(g_command, g_command_data[g_command]);
    }

    gui::end_window();
}

} // namespace command_edit
