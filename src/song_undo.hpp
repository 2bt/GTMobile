#pragma once

namespace song_undo {

void reset();
void sync();
void undo();
void redo();
bool can_undo();
bool can_redo();

} // namespace song_undo
