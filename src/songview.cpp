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

    gui::rgb(0xFF77A8), // 1 portamento up
    gui::rgb(0xFF77A8), // 2 portamento down
    gui::rgb(0xFF77A8), // 3 tone portamento
    gui::rgb(0xFF77A8), // 4 vibrato

    gui::rgb(0x00E436), // 5 attack/decay
    gui::rgb(0x00E436), // 6 sustain/release

    gui::rgb(0xFFA300), // 7 waveform reg
    gui::rgb(0xFFA300), // 8 wavetable ptr
    gui::rgb(0xFFA300), // 9 pulsetable ptr

    gui::rgb(0x29ADFF), // A filtertable ptr
    gui::rgb(0x29ADFF), // B filter control
    gui::rgb(0x29ADFF), // C filter cutoff

    gui::rgb(0x00E436), // D master volume

    gui::rgb(0xFFEC27), // E funk tempo
    gui::rgb(0xFFEC27), // F tempo
};

//constexpr u8vec4 COLOR_SEPARATOR = gui::rgb()
constexpr u8vec4 COLOR_PLAYER_ROW    = gui::rgb(0x382011);
constexpr u8vec4 COLOR_HIGHLIGHT_ROW = gui::rgb(0x1a1a1a);


int                g_chan_num;
int                g_song_row;
std::array<int, 3> g_pattern_nums;

int                g_pattern_row; // ?

bool               g_follow = true;
int                g_song_scroll;
int                g_pattern_scroll;
int                g_row_highlight_step = 4;


} // namespace



void init() {

}

void draw() {


    // buttons
    bool is_playing = app::player().is_playing();
    gui::item_size({32, 32});
    if (gui::button(gui::Icon::PLAY, is_playing)) {
        if (is_playing) {
            app::player().stop_song();
        }
        else {
            app::player().init_song(0, gt::Player::PLAY_BEGINNING);
        }
    }
    gui::same_line();
    if (gui::button(gui::Icon::FOLLOW, g_follow)) {
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


    enum {
        MAX_SONG_ROWS_ON_SCREEN = 8,
        MAX_PATTERN_ROWS_ON_SCREEN = 28,
    };

    if (g_follow) {
        g_pattern_nums = player_pattern_nums;

        int song_row = player_song_rows[g_chan_num];
        g_song_scroll = clamp(max_song_len - MAX_SONG_ROWS_ON_SCREEN, 0, song_row - MAX_SONG_ROWS_ON_SCREEN / 2);

        int pattern_row  = player_pattern_rows[g_chan_num];
        g_pattern_scroll = clamp(max_pattern_len - MAX_PATTERN_ROWS_ON_SCREEN, 0, pattern_row - MAX_PATTERN_ROWS_ON_SCREEN / 2);
    }



    enum {
        ROW_HEIGHT = 11 + 2,
    };
    // put text in the center of the row
    ivec2 text_offset = {0, (ROW_HEIGHT - 7) / 2};


    gui::DrawContext& dc = gui::get_draw_context();

    ivec2 cursor = {0, 32};
    dc.color(gui::DARK_GREY);
    dc.fill({{0, cursor.y}, {8 * 37, 2}});
    cursor.y += 2;
    char line[16];

    // song table
    {
        for (int i = 0; i < MAX_SONG_ROWS_ON_SCREEN; ++i) {
            int row = g_song_scroll + i;

//            // highlight background
//            if (row == g_song_row) {
//                dc.color(gui::DARK_BLUE);
//                dc.fill({ cursor, {8 * 37, ROW_HEIGHT} });
//            }

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
                if (row == player_song_rows[c]) {
                    dc.color(COLOR_PLAYER_ROW);
                    dc.fill({ cursor - ivec2(3, 0), {78, ROW_HEIGHT} });
                }


                uint8_t v = song.songorder[SONG_NR][c][row];

                if (v == gt::LOOPSONG) {
                    sprintf(line, "LOOP %02X", song.songorder[SONG_NR][c][row + 1]);
                }
                else if (v >= gt::TRANSUP) {
                    sprintf(line, "TRANSP +%X ", v & 0xf);
                }
                else if (v >= gt::TRANSDOWN) {
                    sprintf(line, "TRANSP -%X ", v & 0xf);
                }
                else if (v >= gt::REPEAT) {
                    sprintf(line, "REPEAT %X  ", v & 0xf);
                }
                else {
                    sprintf(line, "%02X        ", v);
                }

                dc.color(gui::WHITE);
                dc.text(cursor + text_offset, line);
                cursor.x += 11 * 8;
            }
            cursor.x = 0;
            cursor.y += ROW_HEIGHT;

        }
    }

    {
        // pattern bar
        dc.color(gui::DARK_GREY);
        dc.fill({{0, cursor.y}, {8 * 37, 2}});
        cursor.y += 8;
        cursor.x = 40;
        for (int c = 0; c < 3; ++c) {
            sprintf(line, "%02X        ", g_pattern_nums[c]);
            dc.color(gui::WHITE);
            dc.text(cursor, line);

            // show channel activity
            dc.color(gui::LIGHT_GREY);
            dc.fill({cursor + ivec2(24, 0), ivec2(std::min(sid::chan_level(c) * 1.2f, 1.0f) * 48,  7)});

            cursor.x += 11 * 8;
        }
        cursor.y += 7;
        cursor.y += 8;
        dc.color(gui::DARK_GREY);
        dc.fill({{0, cursor.y - 2}, {8 * 37, 2}});
        cursor.x = 0;
    }


    {
        // patterns
        for (int i = 0; i < MAX_PATTERN_ROWS_ON_SCREEN; ++i) {
            int row = g_pattern_scroll + i;

            // highlight background
            if (row % g_row_highlight_step == 0) {
                dc.color(COLOR_HIGHLIGHT_ROW);
                dc.fill({ cursor, {27, ROW_HEIGHT} });
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
                bool highlight = false;
                if (g_pattern_nums[c] == player_pattern_nums[c] && row == player_pattern_rows[c]) {
                    dc.color(COLOR_PLAYER_ROW);
                    highlight = true;
                }
                else if (row % g_row_highlight_step == 0) {
                    dc.color(COLOR_HIGHLIGHT_ROW);
                    highlight = true;
                }
                if (highlight) {
                    dc.fill({ cursor - ivec2(3, 0), {78, ROW_HEIGHT} });
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
            cursor.y += ROW_HEIGHT;
        }
    }
    dc.color(gui::DARK_GREY);
    dc.fill({{0, cursor.y}, {8 * 37, 2}});

    int h = ROW_HEIGHT * (MAX_SONG_ROWS_ON_SCREEN + MAX_PATTERN_ROWS_ON_SCREEN) + 27;
    dc.fill({{31, 32}, {2, h}});
    dc.fill({{31 + 11 * 8, 32}, {2, h}});
    dc.fill({{31 + 22 * 8, 32}, {2, h}});
    dc.fill({{31 + 33 * 8, 32}, {2, h}});
}



} // namespace songview
