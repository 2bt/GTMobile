#include "songview.hpp"
#include "gtplayer.hpp"
#include "log.hpp"
#include "gui.hpp"
#include "app.hpp"
#include "sid.hpp"
#include <array>


namespace songview {
namespace {


enum {
    SONG_NR = 0,
};

constexpr u8vec4 COLOR_CMDS[16] = {
    gui::rgb(0x000000),

    gui::rgb(0xff77a8), // 1 portamento up
    gui::rgb(0xff77a8), // 2 portamento down
    gui::rgb(0xff77a8), // 3 tone portamento
    gui::rgb(0xff77a8), // 4 vibrato

    gui::rgb(0x00e436), // 5 attack/decay
    gui::rgb(0x00e436), // 6 sustain/release

    gui::rgb(0xffa300), // 7 waveform reg
    gui::rgb(0xffa300), // 8 wavetable ptr
    gui::rgb(0xffa300), // 9 pulsetable ptr

    gui::rgb(0x29adff), // A filtertable ptr
    gui::rgb(0x29adff), // B filter control
    gui::rgb(0x29adff), // C filter cutoff

    gui::rgb(0x00e436), // D master volume

    gui::rgb(0xffec27), // E funk tempo
    gui::rgb(0xffec27), // F tempo
};

constexpr u8vec4 COLOR_SEPARATOR     = gui::rgb(0x444433);
constexpr u8vec4 COLOR_INSTRUMENT    = gui::rgb(0xbbccdd);
constexpr u8vec4 COLOR_HIGHLIGHT_ROW = gui::rgb(0x1f1f10);
constexpr u8vec4 COLOR_PLAYER_ROW    = gui::rgb(0x553300);

// settings
int                g_row_height         = 13;
int                g_row_highlight_step = 4;

std::array<int, 3> g_pattern_nums = { 0, 1, 2 };
int                g_chan_num = 0;

bool               g_follow             = false;
int                g_song_page_length   = 8;
int                g_song_scroll        = 0;
int                g_pattern_scroll     = 0;

int                g_instrument   = 3;
int                g_piano_scroll = 14 * 3;
bool               g_piano_gate   = false;
int                g_piano_note;

//int                g_song_row;
//int                g_pattern_row;



} // namespace



void init() {

}

void draw() {


    // buttons
    gui::item_size({40, 32});
    bool is_playing = app::player().is_playing();
    if (gui::button(gui::Icon::Play, is_playing)) {
        if (is_playing) {
            app::player().stop_song();
        }
        else {
            app::player().init_song(0, gt::Player::PLAY_BEGINNING);
        }
    }
    gui::same_line();
    if (gui::button(gui::Icon::Follow, g_follow)) {
        g_follow = !g_follow;
    }


    // get player position info
    std::array<int, 3> player_song_rows;
    std::array<int, 3> player_pattern_nums;
    std::array<int, 3> player_pattern_rows;
    for (int c = 0; c < 3; ++c) {
        uint8_t  songptr;
        uint8_t  pattnum;
        uint32_t pattptr;
        app::player().get_chan_info(c, songptr, pattnum, pattptr);
        player_song_rows[c]    = std::max<int>(0, songptr - 1);
        player_pattern_nums[c] = pattnum;
        player_pattern_rows[c] = pattptr / 4;
    }

    gt::Song& song = app::song();
    int max_song_len =                    song.songlen[SONG_NR][0];
    max_song_len = std::max(max_song_len, song.songlen[SONG_NR][1]);
    max_song_len = std::max(max_song_len, song.songlen[SONG_NR][2]);
    max_song_len += 1; // include LOOP command

    int max_pattern_len =                       song.pattlen[g_pattern_nums[0]];
    max_pattern_len = std::max(max_pattern_len, song.pattlen[g_pattern_nums[1]]);
    max_pattern_len = std::max(max_pattern_len, song.pattlen[g_pattern_nums[2]]);


    // XXX fix this
    int total_rows = (app::canvas_height() - 130) / g_row_height;
    g_song_page_length = clamp(g_song_page_length, 0, total_rows);
    int pattern_page_length = total_rows - g_song_page_length;



    if (is_playing && g_follow) {
        // auto scroll
        g_pattern_nums = player_pattern_nums;
        g_song_scroll    = player_song_rows[g_chan_num] - g_song_page_length / 2;
        g_pattern_scroll = player_pattern_rows[g_chan_num] - pattern_page_length / 2;
    }
    g_song_scroll    = clamp(g_song_scroll, 0, max_song_len - g_song_page_length);
    g_pattern_scroll = clamp(g_pattern_scroll, 0, max_pattern_len - pattern_page_length);



    gui::DrawContext& dc = gui::get_draw_context();

    ivec2 cursor = {0, 34};
    // dc.color(COLOR_SEPARATOR);
    // dc.fill({cursor, {8 * 37, 2}});
    // cursor.y += 2;

    // // table outline
    // dc.color(COLOR_SEPARATOR);
    // dc.fill({{0, cursor.y}, {8 * 37, 2}});
    // int h = g_row_height * (g_song_page_length + pattern_page_length) + 27;
    // dc.fill({{31, 32}, {2, h}});
    // dc.fill({{31 + 11 * 8, 32}, {2, h}});
    // dc.fill({{31 + 22 * 8, 32}, {2, h}});
    // dc.fill({{31 + 33 * 8, 32}, {2, h}});





    // put text in the center of the row
    ivec2 text_offset = {0, (g_row_height - 7) / 2};


    char line[16];

    // song table
    {
        for (int i = 0; i < g_song_page_length; ++i) {
            int row = g_song_scroll + i;

            cursor.x += 8;
            sprintf(line, "%02X", row);
            dc.color(gui::WHITE);
            dc.text(cursor + text_offset, line);
            cursor.x += 32;

            for (int c = 0; c < 3; ++c) {

                if (row > song.songlen[SONG_NR][c]) {
                    cursor.x += 11 * 8;
                    continue;
                }

                // highlight player position
                if (is_playing && row == player_song_rows[c]) {
                    dc.color(COLOR_PLAYER_ROW);
                    dc.fill({ cursor - ivec2(3, 0), {78, g_row_height} });
                }


                uint8_t v = song.songorder[SONG_NR][c][row];

                if (v == gt::LOOPSONG) {
                    sprintf(line, "LOOP %02X", song.songorder[SONG_NR][c][row + 1]);
                }
                else if (v >= gt::TRANSUP) {
                    sprintf(line, "TRANSP +%X", v & 0xf);
                }
                else if (v >= gt::TRANSDOWN) {
                    sprintf(line, "TRANSP -%X", v & 0xf);
                }
                else if (v >= gt::REPEAT) {
                    sprintf(line, "REPEAT %X", v & 0xf);
                }
                else {
                    sprintf(line, "%02X", v);
                }

                dc.color(gui::WHITE);
                dc.text(cursor + text_offset, line);
                cursor.x += 11 * 8;
            }
            cursor.x = 0;
            cursor.y += g_row_height;

        }
    }

    {
        // pattern bar
        // dc.color(COLOR_SEPARATOR);
        // dc.fill({{0, cursor.y}, {8 * 37, 2}});
        cursor.y += 8;
        cursor.x = 40;
        for (int c = 0; c < 3; ++c) {

            gui::cursor(cursor - ivec2(6, 5));
            gui::item_size({84, 17});
            bool active = app::player().is_channel_active(c);
            if (gui::button(gui::Icon::None, active)) {
                app::player().set_channel_active(c, !active);
            }

            sprintf(line, "%02X", g_pattern_nums[c]);
            dc.color(gui::WHITE);
            dc.text(cursor, line);

            // show channel activity
            dc.color(gui::LIGHT_GREY);
            dc.fill({cursor + ivec2(24, 0), ivec2(std::min(sid::chan_level(c) * 1.2f, 1.0f) * 48,  7)});

            cursor.x += 11 * 8;
        }
        cursor.y += 7;
        cursor.y += 8;
        // dc.color(COLOR_SEPARATOR);
        // dc.fill({{0, cursor.y - 2}, {8 * 37, 2}});
        cursor.x = 0;
    }


    {
        // patterns
        for (int i = 0; i < pattern_page_length; ++i) {
            int row = g_pattern_scroll + i;

            // highlight background
            if (row % g_row_highlight_step == 0) {
                dc.color(COLOR_HIGHLIGHT_ROW);
                dc.fill({ cursor, {27, g_row_height} });
            }

            cursor.x += 8;
            sprintf(line, "%02X", row);
            dc.color(gui::WHITE);
            dc.text(cursor + text_offset, line);
            cursor.x += 32;

            for (int c = 0; c < 3; ++c) {

                if (row >= song.pattlen[g_pattern_nums[c]]) {
                    cursor.x += 28;
                    cursor.x += 20;
                    cursor.x += 40;
                    continue;
                }

                // highlight player position
                if (row % g_row_highlight_step == 0) {
                    dc.color(COLOR_HIGHLIGHT_ROW);
                    dc.fill({ cursor - ivec2(3, 0), {78, g_row_height} });
                }
                if (is_playing && g_pattern_nums[c] == player_pattern_nums[c] && row == player_pattern_rows[c]) {
                    dc.color(COLOR_PLAYER_ROW);
                    dc.fill({ cursor - ivec2(3, 0), {78, g_row_height} });
                }

                uint8_t const* p = song.pattern[g_pattern_nums[c]] + row * 4;
                int note  = p[0];
                int instr = p[1];
                int cmd   = p[2];
                int arg   = p[3];

                dc.color(gui::WHITE);
                if (note == gt::REST) {
                    dc.color(gui::DARK_GREY);
                    dc.text(cursor + text_offset, "...");
                }
                else if (note == gt::KEYOFF) {
                    dc.text(cursor + text_offset, "===");
                }
                else if (note == gt::KEYON) {
                    dc.text(cursor + text_offset, "+++");
                }
                else {
                    sprintf(line, "%c%c%d", "CCDDEFFGGAAB"[note % 12],
                                  "-#-#--#-#-#-"[note % 12],
                                  (note - gt::FIRSTNOTE) / 12);
                    dc.text(cursor + text_offset, line);
                }
                cursor.x += 28;

                if (instr > 0) {
                    sprintf(line, "%02X", instr);
                    dc.color(COLOR_INSTRUMENT);
                    dc.text(cursor + text_offset, line);
                }
                cursor.x += 20;

                if (cmd > 0) {
                    dc.color(COLOR_CMDS[cmd]);
                    sprintf(line, "%X%02X", cmd, arg);
                    dc.text(cursor + text_offset, line);
                }
                cursor.x += 40;
            }
            cursor.x = 0;
            cursor.y += g_row_height;
        }
    }



    // scroll bars
    {
        int page = g_song_page_length;
        int max_scroll = std::max<int>(0, max_song_len - page);
        gui::cursor({298, 34});
        gui::item_size({16, page * g_row_height});
        if (gui::vertical_drag_bar(g_song_scroll, 0, max_scroll, page)) {
            if (is_playing) g_follow = false;
        }
    }
    {
        int page = pattern_page_length;
        int max_scroll = std::max<int>(0, max_pattern_len - page);
        gui::cursor({298, 57 + g_song_page_length * g_row_height});
        gui::item_size({16, page * g_row_height});
        if (gui::vertical_drag_bar(g_pattern_scroll, 0, max_scroll, page)) {
            if (is_playing) g_follow = false;
        }
    }

    {
        // scroll button
        gui::cursor({298, 37 + g_song_page_length * g_row_height});
        gui::item_size({16, 17});
        int dy = 0;
        if (gui::vertical_drag_button(dy)) {
            g_song_page_length += dy;
        }
    }


    /////////////////////////////////////////////////////////////////////////////
    // piano
    enum {
        KEY_HALF_WIDTH = 12,
        KEY_HALF_HEIGHT = 24,
        PIANO_PAGE = app::CANVAS_WIDTH / KEY_HALF_WIDTH,
        PIANO_KEY_COUNT = 14 * 8 - 4,
    };
    int const piano_y = app::canvas_height() - 48;


    gui::cursor({app::CANVAS_WIDTH - 200, piano_y - 18});
    gui::item_size({200, 16});
    gui::horizontal_drag_bar(g_piano_scroll, 0, PIANO_KEY_COUNT - PIANO_PAGE, PIANO_PAGE);


    auto loop_keys = [&](auto f) {
        for (int i = -1; i < 30; i += 1) {
            constexpr int NOTE_LUT[14] = { 0, 1, 2, 3, 4, -1, 5, 6, 7, 8, 9, 10, 11, -1, };
            int n = i + g_piano_scroll;
            int note = NOTE_LUT[n % 14];
            if (note == -1) continue;
            f(i, n, note + n / 14 * 12);
        }
    };

    // check touch
    bool prev_gate = g_piano_gate;
    int  prev_note = g_piano_note;

    g_piano_gate = false;

    if (!gui::has_active_item() && gui::touch::pressed()) {
        loop_keys([&](int i, int n, int note) {
            gui::Box b = {
                { i * KEY_HALF_WIDTH, piano_y },
                { KEY_HALF_WIDTH * 2, KEY_HALF_HEIGHT },
            };
            if (n % 2 == 0) b.pos.y += KEY_HALF_HEIGHT;
            if (gui::touch::touched(b)) {
                g_piano_gate = true;
                g_piano_note = note;
            }
        });
    }
    if (g_piano_gate && (!prev_gate || g_piano_note != prev_note)) {
        app::player().play_test_note(g_piano_note + gt::FIRSTNOTE, g_instrument, g_chan_num);
    }
    if (!g_piano_gate && prev_gate) {
        app::player().release_note(g_chan_num);
    }


    // render
    loop_keys([&](int i, int n, int note) {
        if (n % 2 == 0) {
            dc.color(gui::rgb(0xbbbbbb));
            if (g_piano_gate && g_piano_note == note) {
                // key is pressed
                dc.color(gui::rgb(0xff4422));
            }
            gui::Box b = {
                { i * KEY_HALF_WIDTH, piano_y },
                { KEY_HALF_WIDTH * 2, KEY_HALF_HEIGHT * 2 },
            };
            dc.box(b, gui::BoxStyle::PianoKey);
            if (note % 12 == 0) {
                char str[] = { char('0' + note / 12), '\0' };
                dc.color(gui::DARK_GREY);
                dc.text({b.pos.x + KEY_HALF_WIDTH - 4, piano_y + 33}, str);
            }
        }
    });
    loop_keys([&](int i, int n, int note) {
        if (n % 2 == 1) {
            dc.color(gui::rgb(0x333333));
            if (g_piano_gate && g_piano_note == note) {
                // key is pressed
                dc.color(gui::rgb(0xff4422));
            }
            gui::Box b = {
                { i * KEY_HALF_WIDTH, piano_y },
                { KEY_HALF_WIDTH * 2, KEY_HALF_HEIGHT },
            };
            dc.box(b, gui::BoxStyle::PianoKey);
        }
    });
}

} // namespace songview