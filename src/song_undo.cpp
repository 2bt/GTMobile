#include "song_undo.hpp"

#include "app.hpp"
#include "gtsong.hpp"
#include "log.hpp"

#include <array>
#include <cstring>


namespace song_undo {
namespace {

constexpr int MAX_LEVELS = 64;

gt::Song& g_song = app::song();

std::array<gt::Song, MAX_LEVELS> g_undo;
std::array<gt::Song, MAX_LEVELS> g_redo;
int                              g_undo_size = 0;
int                              g_redo_size = 0;
gt::Song                         g_anchor;
bool                             g_anchor_valid = false;

bool equals(gt::Song const& a, gt::Song const& b) {
    return std::memcmp(&a, &b, sizeof(a)) == 0;
}

} // namespace

void reset() {
    g_undo_size    = 0;
    g_redo_size    = 0;
    g_anchor_valid = false;
}

void sync() {
    if (!g_anchor_valid) {
        g_anchor_valid = true;
        g_anchor       = g_song;
    }

    if (equals(g_anchor, g_song)) return;

    if (g_undo_size == 0 || !equals(g_undo[g_undo_size - 1], g_anchor)) {
        if (g_undo_size >= MAX_LEVELS) {
            for (int i = 1; i < MAX_LEVELS; ++i) g_undo[i - 1] = g_undo[i];
            g_undo[MAX_LEVELS - 1] = g_anchor;
        }
        else {
            g_undo[g_undo_size++] = g_anchor;
        }
    }
    g_redo_size = 0;
    g_anchor    = g_song;
}

void undo() {
    while (g_undo_size > 0) {
        gt::Song const& prev = g_undo[--g_undo_size];
        if (equals(prev, g_song)) continue;
        g_redo[g_redo_size++] = g_song;
        g_anchor = g_song = prev;
        return;
    }
}

void redo() {
    while (g_redo_size > 0) {
        gt::Song const& next = g_redo[--g_redo_size];
        if (equals(next, g_song)) continue;
        g_undo[g_undo_size++] = g_song;
        g_anchor = g_song = next;
        return;
    }
}

bool can_undo() {
    return g_undo_size > 0 && !equals(g_undo[g_undo_size - 1], g_song);
}

bool can_redo() {
    return g_redo_size > 0 && !equals(g_redo[g_redo_size - 1], g_song);
}

} // namespace song_undo
