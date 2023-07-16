#include "songview.hpp"
#include "gui.hpp"
#include "app.hpp"


namespace songview {
namespace {


int g_selected_chan = 0;


} // namespace



void init() {

}

void draw() {

//    {
//        // draw mock img
//        gui::DrawContext dc;
//        dc.rect({}, g_mock_img.size(), {});
//        gfx::draw(dc, g_mock_img);
//    }


    gui::DrawContext& dc = gui::get_draw_context();

    enum {
        SONG_NR = 0,

        MAX_SONG_ROWS_ON_SCREEN = 8,
        MAX_PATTERN_ROWS_ON_SCREEN = 28,
    };

    enum {
        ROW_HEIGHT = 11,
    };

    gt::Song& song = app::song();

    uint8_t  songptr;
    uint8_t  pattnum;
    uint32_t pattptr;
    app::player().get_chan_info(g_selected_chan, songptr, pattnum, pattptr);

    char line[16];
    ivec2 cursor = {0, 32};

    cursor.y += 2;
    // song table
    {
        int len = song.songlen[SONG_NR][g_selected_chan];
        int current_row = std::max<int>(0, songptr - 1);

        int start_row = clamp(len - MAX_SONG_ROWS_ON_SCREEN, 0, current_row - MAX_SONG_ROWS_ON_SCREEN / 2);

        for (int i = 0; i < MAX_SONG_ROWS_ON_SCREEN; ++i) {
            int row = start_row + i;
            if (row >= len) {
                cursor.x = 0;
                cursor.y += ROW_HEIGHT;
                continue;
            }

            // highlight background
            if (row == current_row) {
                dc.set_color(gui::DARK_BLUE);
                dc.box({ cursor - ivec2(0, 2), {8 * 37, ROW_HEIGHT} });
            }
            dc.set_color(gui::LIGHT_GREY);

            cursor.x += 8;
            sprintf(line, "%02X", row);
            dc.text(cursor, line);
            cursor.x += 32;

            for (int c = 0; c < 3; ++c) {
                uint8_t v = song.songorder[SONG_NR][c][row];

                if (v == gt::LOOPSONG) {
                    sprintf(line, "RESTART    ");
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

                dc.text(cursor, line);
                cursor.x += 11 * 8;
            }
            cursor.x = 0;
            cursor.y += ROW_HEIGHT;

        }
    }

    cursor.x = 0;
    cursor.y += ROW_HEIGHT;


    int pattern_len = song.pattlen[pattnum];
    int current_row = pattptr / 4;
    if (current_row > pattern_len) current_row = pattern_len;



    // patterns
    int start_row = clamp(pattern_len - MAX_PATTERN_ROWS_ON_SCREEN, 0, current_row - MAX_PATTERN_ROWS_ON_SCREEN / 2);

    for (int i = 0; i < MAX_PATTERN_ROWS_ON_SCREEN; ++i) {
        int row = start_row + i;
        if (row >= pattern_len) break;

        // highlight background
        if (row == current_row) {
            dc.set_color(gui::DARK_BLUE);
            dc.box({ cursor - ivec2(0, 2), {8 * 37, ROW_HEIGHT} });
        }
        dc.set_color(gui::LIGHT_GREY);

        cursor.x += 8;
        sprintf(line, "%02X", row);
        dc.text(cursor, line);
        cursor.x += 32;

        for (int c = 0; c < 3; ++c) {

            uint8_t  songptr;
            uint8_t  pattnum;
            uint32_t pattptr;
            app::player().get_chan_info(c, songptr, pattnum, pattptr);
            uint8_t const* p = song.pattern[pattnum] + row * 4;

            if (*p == 0xff) break;

            int note  = p[0];
            int instr = p[1];
            int cmd   = p[2];
            int arg   = p[3];

            if (note == gt::REST) {
                dc.set_color(gui::DARK_GREY);
                dc.text(cursor, "...");
            }
            else if (note == gt::KEYOFF) {
                dc.set_color(gui::LIGHT_GREY);
                dc.text(cursor, "===");
            }
            else if (note == gt::KEYOFF) {
                dc.set_color(gui::LIGHT_GREY);
                dc.text(cursor, "===");
            }
            else {
                dc.set_color(gui::LIGHT_GREY);
                sprintf(line, "%c%c%d", "CCDDEFFGGAAB"[note % 12],
                              "-#-#--#-#-#-"[note % 12],
                              (note - gt::FIRSTNOTE) / 12);
                dc.text(cursor, line);
            }
            cursor.x += 28;

            if (instr == 0) {
                dc.set_color(gui::DARK_GREY);
                dc.text(cursor, "..");
            }
            else {
                dc.set_color(gui::LIGHT_GREY);
                sprintf(line, "%02X", instr);
                dc.text(cursor, line);
            }
            cursor.x += 20;

            if (cmd == 0) {
                dc.set_color(gui::DARK_GREY);
                dc.text(cursor, "...");
            }
            else {
                dc.set_color(gui::LIGHT_GREY);
                sprintf(line, "%X%02X", cmd, arg);
                dc.text(cursor, line);
            }
            cursor.x += 40;
        }
        cursor.x = 0;
        cursor.y += ROW_HEIGHT;
    }


    gui::end_frame();
}



} // namespace songview
