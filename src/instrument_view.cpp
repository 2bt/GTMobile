#include "instrument_view.hpp"
#include "piano.hpp"
#include "app.hpp"
#include "gui.hpp"


namespace instrument_view {
namespace {



} // namespace


void draw() {

    gt::Song& song = app::song();
    int instr_nr = piano::instrument();
    gt::Instr& instr = song.instr[instr_nr];

    gui::align(gui::Align::Left);
    gui::item_size({ 2 * 8 + 12, 20 });

    char str[32];
    sprintf(str, "%02X", instr_nr);
    if (gui::button(str)) {

    }

    gui::same_line();
    gui::item_size({ 16 * 8 + 12, 20 });
    gui::input_text(instr.name);


    piano::draw();
}


} // namespace instrument_view