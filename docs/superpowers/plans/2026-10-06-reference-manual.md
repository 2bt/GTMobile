# Reference Manual Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace `docs/manual.markdown` with a complete GTMobile control reference (icons + mode screenshots + GoatTracker 2 differences), using source as the source of truth.

**Architecture:** One Jekyll page. A small Python script slices 16×16 icons from `assets/gui.png` using `enum class Icon` in `src/gui.hpp`. Manual chapters follow app order; each mode is a screenshot (if present) plus a control list. The author captures PPMs; this plan writes copy and converts/places PNGs when files exist. Do not wait on screenshots to write lists.

**Tech Stack:** Jekyll (existing `docs/`), Python 3 stdlib + ImageMagick (`convert` or `magick`), GitHub Pages / minima theme.

## Global Constraints

- One page: `docs/manual.markdown`, permalink `/manual/`.
- No beginner tutorial and no video placeholder.
- Source of truth: `gui::button` handlers in `src/app.cpp`, `src/piano.cpp`, `src/project_view.cpp`, `src/song_view.cpp`, `src/command_edit.cpp`, `src/instrument_view.cpp`, `src/instrument_manager_view.cpp`, `src/settings_view.cpp`.
- Icon files: `docs/assets/icons/<lowercase-enum-name>.png` (e.g. `copy.png`, `playpause.png`).
- Shots: `docs/assets/shots/<id>.png` per spec checklist; raw PPMs in `docs/shots-raw/` (gitignored); never commit `shot-*.ppm`.
- If a shot PNG is missing, omit the `<img>` (do not leave a broken image). Still write the control list.
- Icon buttons: 16px pixelated PNG + English name + one present-tense sentence. Text buttons: on-screen label, no glyph. Custom font glyphs (gate off/on, transpose): English name only.
- Copy/Paste/Close: full sentence once, then “same as …”.
- No opcode bible. Command/table editors: what the buttons do, not every legal byte.
- Do not change application code.
- English, brief, present tense.
- No new documentation toolchain.
- Fix stale “no SID export” copy. SID, SNG, WAV, OGG exist.

## File structure

- Create: `docs/extract-icons.py` — parse Icon enum, crop atlas.
- Create: `docs/assets/icons/*.png` — generated, committed.
- Create: `docs/assets/shots/*.png` — only when PPMs are supplied.
- Create: `docs/shots-raw/` — drop zone; gitignored.
- Modify: `.gitignore` — ignore raw PPMs.
- Modify: `docs/assets/main.scss` — `.gui-icon`.
- Modify: `docs/manual.markdown` — the manual.
- Modify: `todo.md` — mark GoatTracker differences done after pass 2.

Helper for a control line (use in every chapter):

```html
- <img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy** — Copies the selected region.
```

Helper for a shot (skip the `<p>`/`<img>` if the file is absent):

```html
<p><img src="{{ '/assets/shots/always-on.png' | relative_url }}" alt="Always-on controls"></p>
```

---

### Task 1: Ignore raw shots and extract icons

**Files:**
- Modify: `.gitignore`
- Create: `docs/extract-icons.py`
- Create: `docs/assets/icons/*.png` (script output)
- Create: `docs/shots-raw/.gitkeep` is not wanted; do not add a keep file.

**Interfaces:**
- Consumes: `src/gui.hpp` `enum class Icon`; `assets/gui.png`
- Produces: `parse_icons(text) -> list[tuple[str,int]]`; PNG files named from enum (`PlayPause` → `playpause.png`)

- [ ] **Step 1: Write a failing parser check**

Create `docs/extract-icons.py` with `parse_icons` and a `__main__` that exits 1 until the rest is implemented. Put this at the top of the file:

```python
#!/usr/bin/env python3
"""Slice GUI icons from assets/gui.png using enum class Icon in src/gui.hpp."""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GUI_HPP = ROOT / "src" / "gui.hpp"
ATLAS = ROOT / "assets" / "gui.png"
OUT_DIR = Path(__file__).resolve().parent / "assets" / "icons"

ENUM_RE = re.compile(r"enum class Icon\s*\{([^}]+)\}", re.S)
ENTRY_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)(?:\s*=\s*(\d+))?")


def parse_icons(text: str) -> list[tuple[str, int]]:
    m = ENUM_RE.search(text)
    if not m:
        raise SystemExit("enum class Icon not found in gui.hpp")
    value = None
    icons: list[tuple[str, int]] = []
    for raw in m.group(1).splitlines():
        line = raw.split("//", 1)[0].strip().rstrip(",")
        if not line:
            continue
        em = ENTRY_RE.match(line)
        if not em:
            raise SystemExit(f"unparsed Icon line: {raw!r}")
        name = em.group(1)
        if em.group(2) is not None:
            value = int(em.group(2))
        else:
            if value is None:
                raise SystemExit(f"Icon {name} has no value")
            value += 1
        icons.append((name, value))
    return icons


def self_check(icons: list[tuple[str, int]]) -> None:
    by_name = dict(icons)
    assert by_name["Decrease"] == 144, by_name.get("Decrease")
    assert by_name["AddRowAbove"] == 160
    assert by_name["Copy"] == 208
    assert by_name["Noise"] == 224
    assert by_name["Highpass"] == 234
    print(f"ok: {len(icons)} icons")
```

Do not add crop/`main` yet. Run:

```bash
python3 -c "import ast,pathlib; ast.parse(pathlib.Path('docs/extract-icons.py').read_text())"
```

Expected: success (syntax ok). Then:

```bash
python3 -c "
from importlib.machinery import SourceFileLoader
m = SourceFileLoader('e','docs/extract-icons.py').load_module()
print('parse_icons' in dir(m))
"
```

Expected: `True`.

- [ ] **Step 2: Finish the script (crop + CLI)**

Append:

```python
def crop_icon(name: str, index: int) -> Path:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    dest = OUT_DIR / (name.lower() + ".png")
    x = (index % 16) * 16
    y = (index // 16) * 16
    geom = f"16x16+{x}+{y}"
    cmds = [
        ["magick", str(ATLAS), "-crop", geom, "+repage", str(dest)],
        ["convert", str(ATLAS), "-crop", geom, "+repage", str(dest)],
    ]
    last_err = None
    for cmd in cmds:
        try:
            subprocess.run(cmd, check=True, capture_output=True)
            return dest
        except (FileNotFoundError, subprocess.CalledProcessError) as e:
            last_err = e
    raise SystemExit(f"ImageMagick crop failed for {name}: {last_err}")


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--check", action="store_true", help="parse and assert known indices")
    args = p.parse_args()
    icons = parse_icons(GUI_HPP.read_text())
    self_check(icons)
    if args.check:
        return 0
    if not ATLAS.is_file():
        raise SystemExit(f"missing atlas {ATLAS}")
    for name, index in icons:
        crop_icon(name, index)
        print(name.lower())
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Step 3: Run the check (must pass)**

```bash
python3 docs/extract-icons.py --check
```

Expected: `ok: N icons` where N is the number of entries in `enum class Icon`. The asserts are the contract. If an assert fails, the enum changed — fix the expected values to match `src/gui.hpp`, do not invent indices.

- [ ] **Step 4: Extract PNGs**

```bash
python3 docs/extract-icons.py
ls docs/assets/icons | wc -l
```

Expected: one PNG per icon; `copy.png` and `playpause.png` exist. Spot-check `copy.png` is 16×16:

```bash
identify docs/assets/icons/copy.png
```

Expected: `16x16`. If `identify` is missing, `file docs/assets/icons/copy.png` is enough.

- [ ] **Step 5: Gitignore raw shots**

Append to `.gitignore`:

```
/docs/_site
/docs/shots-raw/
/shot-*.ppm
```

Keep the existing `/docs/_site` line (do not duplicate it). Add only the two new lines.

- [ ] **Step 6: Commit**

```bash
git add .gitignore docs/extract-icons.py docs/assets/icons
git commit -m "$(cat <<'EOF'
Add GUI icon extract script and atlas slices for the manual.

EOF
)"
```

---

### Task 2: Icon CSS and manual skeleton

**Files:**
- Modify: `docs/assets/main.scss`
- Modify: `docs/manual.markdown`

**Interfaces:**
- Consumes: `.gui-icon` class used by later chapters
- Produces: rewritten `docs/manual.markdown` with intro + empty `##` headings only (no `TODO` strings)

- [ ] **Step 1: Add CSS**

Replace `docs/assets/main.scss` with:

```scss
---
# Only the main Sass file needs front matter (the dashes are enough)
---

@import "minima";

.gui-icon {
  width: 16px;
  height: 16px;
  image-rendering: pixelated;
  vertical-align: middle;
}
```

- [ ] **Step 2: Replace the manual with skeleton + intro**

Overwrite `docs/manual.markdown` with exactly:

```markdown
---
layout: page
title: Manual
permalink: /manual/
---

GTMobile is a C64 SID tracker for Android. It is based on and largely compatible with [GoatTracker 2](https://sourceforge.net/projects/goattracker2/). You edit one song at a time.

This page is a control reference: each screen (and each selection mode) is listed with what the buttons do. GoatTracker users should also read [Differences from GoatTracker 2](#differences-from-goattracker-2).

Tap the splash screen to start. After that the layout is always: tabs and undo/redo at the top, the current view in the middle, the piano above the playback buttons at the bottom.

## Always-on controls

## Piano

## Project

## Song

## Command editor

## Instrument

## Instrument manager

## Settings

## Differences from GoatTracker 2
```

- [ ] **Step 3: Confirm no leftover TODO**

```bash
grep -n TODO docs/manual.markdown || true
```

Expected: no matches.

- [ ] **Step 4: Commit**

```bash
git add docs/assets/main.scss docs/manual.markdown
git commit -m "$(cat <<'EOF'
Rewrite the manual intro and section outline.

EOF
)"
```

---

### Task 3: Always-on controls

**Files:**
- Modify: `docs/manual.markdown` section `## Always-on controls`
- Optionally add: `docs/assets/shots/always-on.png` if a PPM exists

**Interfaces:**
- Consumes: `src/app.cpp` `draw()` tabs/undo and `draw_play_buttons()`
- Produces: filled Always-on section

- [ ] **Step 1: If a matching PPM exists, convert it**

Look in repo root and `docs/shots-raw/` for author captures. If one is the always-on shot:

```bash
mkdir -p docs/assets/shots docs/shots-raw
convert docs/shots-raw/shot-NNNN.ppm docs/assets/shots/always-on.png
```

Use `magick` if `convert` is missing. Skip this step when no PPM exists.

- [ ] **Step 2: Write the section**

Replace `## Always-on controls` plus its empty body with:

```markdown
## Always-on controls

These controls stay on screen in every view.

<p><img src="{{ '/assets/shots/always-on.png' | relative_url }}" alt="Tabs, undo, and playback"></p>

**Tabs** (top row):

- **PROJECT** — Opens the project view (file and song metadata).
- **SONG** — Opens the song view (order list and patterns).
- **INSTR** — Opens the instrument editor. Tap again to open the instrument manager.
- <img class="gui-icon" src="{{ '/assets/icons/settings.png' | relative_url }}" alt=""> **Settings** — Opens settings.

**History** (top right):

- <img class="gui-icon" src="{{ '/assets/icons/undo.png' | relative_url }}" alt=""> **Undo** — Reverts the last song edit. Disabled when there is nothing to undo.
- <img class="gui-icon" src="{{ '/assets/icons/redo.png' | relative_url }}" alt=""> **Redo** — Re-applies an undone edit. Disabled when there is nothing to redo.

**Playback** (bottom row, under the piano):

- <img class="gui-icon" src="{{ '/assets/icons/fastbackward.png' | relative_url }}" alt=""> **Fast backward** — While the song is playing, jumps toward the start of the song; a longer press restarts the current pattern. When stopped, steps the order position backward (or to the start of the pattern).
- <img class="gui-icon" src="{{ '/assets/icons/stop.png' | relative_url }}" alt=""> **Stop** — Stops playback and sets the restart point to the current order row.
- <img class="gui-icon" src="{{ '/assets/icons/playpause.png' | relative_url }}" alt=""> **Play/pause** — Starts playback from the current start position, or pauses if already playing.
- <img class="gui-icon" src="{{ '/assets/icons/follow.png' | relative_url }}" alt=""> **Follow** — When on, the pattern view follows playback. When off, the cursor stays put.
- <img class="gui-icon" src="{{ '/assets/icons/loop.png' | relative_url }}" alt=""> **Loop** — When on, playback loops the current pattern instead of advancing in the order list.
- <img class="gui-icon" src="{{ '/assets/icons/fastforward.png' | relative_url }}" alt=""> **Fast forward** — While the song is playing, jumps forward. When stopped, advances the order position (wraps to the start).
```

If `docs/assets/shots/always-on.png` is missing, delete the `<p><img ...></p>` line only.

- [ ] **Step 3: Commit**

```bash
git add docs/manual.markdown docs/assets/shots/always-on.png 2>/dev/null || git add docs/manual.markdown
git commit -m "$(cat <<'EOF'
Document always-on tabs, undo, and playback controls.

EOF
)"
```

---

### Task 4: Piano

**Files:**
- Modify: `docs/manual.markdown` `## Piano`
- Optionally: `docs/assets/shots/piano.png`, `docs/assets/shots/piano-select.png`

**Interfaces:**
- Consumes: `src/piano.cpp`
- Produces: Piano section

- [ ] **Step 1: Convert shots if present** (`piano`, `piano-select`) into `docs/assets/shots/`.

- [ ] **Step 2: Write the section**

```markdown
## Piano

The piano is available in every view. It plays the current instrument through the SID emulator.

<p><img src="{{ '/assets/shots/piano.png' | relative_url }}" alt="Piano keyboard"></p>

- **Instrument** (left, shows index and name) — Opens the instrument picker. In **Song** view, long-press this button to set the piano instrument from the selected pattern row (if that row has an instrument).
- **Octave bar** — Drag horizontally to scroll which keys are visible.

<p><img src="{{ '/assets/shots/piano-select.png' | relative_url }}" alt="Instrument picker"></p>

**Instrument picker:**

- Tap an instrument to select it and close the picker. Empty instruments (no table pointers) are shaded.
- **CLOSE** — Closes the picker without changing the instrument (same as Close elsewhere).
```

Omit missing `<img>` tags.

- [ ] **Step 3: Commit**

```bash
git add docs/manual.markdown docs/assets/shots/piano.png docs/assets/shots/piano-select.png
git commit -m "$(cat <<'EOF'
Document the piano and instrument picker.

EOF
)"
```

If some shot files are missing, `git add` only the files that exist.

---

### Task 5: Project

**Files:**
- Modify: `docs/manual.markdown` `## Project`
- Optionally shots: `project-files`, `project-demos`, `project-export`, `project-confirm`

**Interfaces:**
- Consumes: `src/project_view.cpp`, `src/app.cpp` `draw_confirm`

- [ ] **Step 1: Convert shots if present.**

- [ ] **Step 2: Write the section**

```markdown
## Project

Song metadata and files. GTMobile edits one song at a time.

<p><img src="{{ '/assets/shots/project-files.png' | relative_url }}" alt="Project files tab"></p>

**Fields**

- **TITLE** — Song title (written into the `.sng` / SID header).
- **AUTHOR** — Author name.
- **RELEASED** — Copyright / release note.
- **FILE** (FILES tab only) — Name of the current song file. LOAD, SAVE, DELETE, and EXPORT use this name.

**Tabs**

- **FILES** — User songs stored on the device.
- **DEMOS** — Bundled demo songs (read-only load).

**FILES toolbar**

- **LOAD** — Loads the selected file after confirm (“lose changes”).
- **SAVE** — Saves under the FILE name. If that file already exists, confirms overwrite.
- **DELETE** — Deletes the selected file after confirm.
- **IMPORT** — Imports a song from the system file picker (Android). Confirms losing the current song.
- **EXPORT** — Opens the export window. Disabled until FILE has a name.
- **RESET** — Clears the song after confirm.

<p><img src="{{ '/assets/shots/project-demos.png' | relative_url }}" alt="Project demos tab"></p>

**DEMOS toolbar**

- **LOAD** — Loads the selected demo after confirm.

<p><img src="{{ '/assets/shots/project-export.png' | relative_url }}" alt="Export window"></p>

**Export window**

- Format row — **SNG**, **SID**, **WAV**, or **OGG**.
- **EXPORT** — Writes that format and hands the file to the system share/export flow. WAV and OGG render in the background with a progress bar.
- **CLOSE** — Closes the window (same as other Close buttons).
- **CANCEL** — Aborts an in-progress WAV/OGG render.

<p><img src="{{ '/assets/shots/project-confirm.png' | relative_url }}" alt="Confirm dialog"></p>

**Confirm / alert** (used by load, save overwrite, delete, reset, import, and errors)

- **OK** — Accepts the action (or dismisses an alert).
- **CANCEL** — Dismisses without changing the song (confirm only).
```

- [ ] **Step 3: Commit** with message `Document the project view, export, and confirms.`

---

### Task 6: Song — order list

**Files:**
- Modify: `docs/manual.markdown` `## Song` (create the Song intro + order-list subsections; leave pattern for Task 7)
- Optionally shots: `song-order-cell`, `song-order-region`, `song-order-edit`

**Interfaces:**
- Consumes: `src/song_view.cpp` order-edit buttons (`EditMode::Song`, `SongMark`, `init_order_edit` / `draw_order_edit`)

- [ ] **Step 1: Convert shots if present.**

- [ ] **Step 2: Write Song intro + order list** (replace `## Song` empty heading)

```markdown
## Song

A song is an **order list** (which patterns play on each of the three SID voices) plus the **patterns** themselves. Instruments are numbered `01`–`3F`. Patterns are numbered `00`–`CF`.

**Tap** an order-list cell to select it (that row’s three patterns appear below). **Long-press and drag** to select a rectangle of cells.

### Order list — one cell

<p><img src="{{ '/assets/shots/song-order-cell.png' | relative_url }}" alt="Order list single cell buttons"></p>

- <img class="gui-icon" src="{{ '/assets/icons/paste.png' | relative_url }}" alt=""> **Paste** — Pastes a copied order-list region starting at this cell.
- <img class="gui-icon" src="{{ '/assets/icons/deleterow.png' | relative_url }}" alt=""> **Delete row** — Deletes this order row on all voices. Disabled if only one row remains.
- <img class="gui-icon" src="{{ '/assets/icons/addrowabove.png' | relative_url }}" alt=""> **Add row above** — Inserts a copy of this row above. Disabled if the order list is full.
- <img class="gui-icon" src="{{ '/assets/icons/addrowbelow.png' | relative_url }}" alt=""> **Add row below** — Inserts a copy of this row below.
- <img class="gui-icon" src="{{ '/assets/icons/jumpback.png' | relative_url }}" alt=""> **Loop here** — Marks this row as the loop start when the song repeats. The row shows a small loop mark.
- <img class="gui-icon" src="{{ '/assets/icons/clone.png' | relative_url }}" alt=""> **Clone pattern** — Copies this cell’s pattern into a free pattern slot and points the cell at the copy (only if the source pattern is not empty).
- <img class="gui-icon" src="{{ '/assets/icons/edit.png' | relative_url }}" alt=""> **Edit** — Opens the pattern index window for this cell.

### Order list — region

<p><img src="{{ '/assets/shots/song-order-region.png' | relative_url }}" alt="Order list region buttons"></p>

- <img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy** — Copies the selected order cells.
- <img class="gui-icon" src="{{ '/assets/icons/edit.png' | relative_url }}" alt=""> **Edit** — Opens the pattern index window for every cell in the selection (same as Edit above).

### Pattern index window

<p><img src="{{ '/assets/shots/song-order-edit.png' | relative_url }}" alt="Pattern index window"></p>

- Tap a pattern number to assign it to the selected cell(s). Empty patterns are shaded. Long-press a number to drag-reorder pattern slots.
- **TRANSPOSE** slider — Sets order-list transpose for the selected cell(s) (`-F` … `+E`).
- **CLOSE** — Closes the window.
```

- [ ] **Step 3: Commit** with message `Document the song order list and pattern index window.`

---

### Task 7: Song — patterns

**Files:**
- Modify: `docs/manual.markdown` (append under Song, before `## Command editor`)
- Optionally shots: `song-pattern-cell`, `song-pattern-region`, `song-pattern-length`, `song-autostep`

**Interfaces:**
- Consumes: `src/song_view.cpp` pattern bar + `EditMode::Pattern` / `PatternMark`

- [ ] **Step 1: Convert shots if present.**

- [ ] **Step 2: Append**

```markdown
### Pattern channels

Above the pattern rows, three buttons show the current pattern index per voice. Tap one to mute or unmute that voice. The small bar is the SID envelope level.

### Pattern — one row

<p><img src="{{ '/assets/shots/song-pattern-cell.png' | relative_url }}" alt="Pattern single row buttons"></p>

**Tap** a pattern row to select it. **Long-press and drag** to select a region.

- <img class="gui-icon" src="{{ '/assets/icons/paste.png' | relative_url }}" alt=""> **Paste** — Pastes copied notes and/or commands starting at this row (same as order-list Paste, for pattern data).
- <img class="gui-icon" src="{{ '/assets/icons/changelength.png' | relative_url }}" alt=""> **Pattern length** — Opens the pattern length window.
- <img class="gui-icon" src="{{ '/assets/icons/deleterow.png' | relative_url }}" alt=""> **Delete row** — Shifts later rows up and clears the last row (does not change pattern length).
- <img class="gui-icon" src="{{ '/assets/icons/addrowabove.png' | relative_url }}" alt=""> **Add row above** — Shifts this row and below down, leaving an empty row here.

Note tools:

- <img class="gui-icon" src="{{ '/assets/icons/x.png' | relative_url }}" alt=""> **Clear note** — Clears the note and instrument on this row (auto-steps if auto-step is on).
- **Gate off / gate on** — Writes a gate-off; tap again on a gate-off row to write gate-on.
- <img class="gui-icon" src="{{ '/assets/icons/record.png' | relative_url }}" alt=""> **Record** — When on, piano keys write the current instrument’s note into this row (and auto-step).
- <img class="gui-icon" src="{{ '/assets/icons/autostep.png' | relative_url }}" alt=""> **Auto-step** — When on, note entry advances the cursor. Long-press opens the auto-step window.

Command tools:

- <img class="gui-icon" src="{{ '/assets/icons/x.png' | relative_url }}" alt=""> **Clear command** — Clears the pattern command and data.
- <img class="gui-icon" src="{{ '/assets/icons/edit.png' | relative_url }}" alt=""> **Edit command** — Opens the [command editor](#command-editor). If the command is a table pointer (wave/pulse/filter table), **long-press** jumps to that instrument and table in the instrument view.

Playback from cursor:

- <img class="gui-icon" src="{{ '/assets/icons/play.png' | relative_url }}" alt=""> **Play from here** — Starts the song at this order row and pattern row.
- <img class="gui-icon" src="{{ '/assets/icons/playrow.png' | relative_url }}" alt=""> **Play row** — Plays this pattern row once, then auto-steps.

### Pattern length window

<p><img src="{{ '/assets/shots/song-pattern-length.png' | relative_url }}" alt="Pattern length window"></p>

- **LENGTH** slider — Sets this pattern’s row count.
- **RESIZE EMPTY PATTERNS** — Sets every empty pattern to this length.
- **SHRINK** — Drops every other row (halves length). Disabled at length 1.
- **EXPAND** — Inserts a blank row after each row (doubles length). Disabled if that would exceed the maximum.
- **CLOSE** — Closes and clears unused rows beyond the new length.

### Auto-step window

<p><img src="{{ '/assets/shots/song-autostep.png' | relative_url }}" alt="Auto-step window"></p>

- **STEP** slider — How many rows to advance on note entry (1–8).
- **CLOSE** — Closes the window.

### Pattern — region

<p><img src="{{ '/assets/shots/song-pattern-region.png' | relative_url }}" alt="Pattern region buttons"></p>

- <img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy** — Copies notes and commands.
- <img class="gui-icon" src="{{ '/assets/icons/x.png' | relative_url }}" alt=""> **Clear** — Clears notes and commands in the region.
- <img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy notes** — Copies only notes and instruments.
- <img class="gui-icon" src="{{ '/assets/icons/x.png' | relative_url }}" alt=""> **Clear notes** — Clears notes and instruments.
- **Transpose up** / **Transpose down** — Moves notes in the region by one semitone (clamped to the valid note range).
- <img class="gui-icon" src="{{ '/assets/icons/piano.png' | relative_url }}" alt=""> **Set instrument** — Sets every existing instrument index in the region to the piano’s current instrument.
- <img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy commands** — Copies only commands.
- <img class="gui-icon" src="{{ '/assets/icons/x.png' | relative_url }}" alt=""> **Clear commands** — Clears commands and data.
- <img class="gui-icon" src="{{ '/assets/icons/edit.png' | relative_url }}" alt=""> **Edit command** — Opens the command editor and applies the result to every row in the region.
```

- [ ] **Step 3: Commit** with message `Document pattern editing, length, and auto-step.`

---

### Task 8: Command editor

**Files:**
- Modify: `docs/manual.markdown` `## Command editor`
- Optionally: `docs/assets/shots/song-command.png`

**Interfaces:**
- Consumes: `src/command_edit.cpp`

- [ ] **Step 1: Convert shot if present.**

- [ ] **Step 2: Write the section**

```markdown
## Command editor

Opens from a pattern row, a pattern region, or a wave-table command row. Left: command list. Right: parameters for the selected command. Values are stored with the song; this window edits them.

<p><img src="{{ '/assets/shots/song-command.png' | relative_url }}" alt="Command editor"></p>

- Tap a command name to select it (**DO NOTHING**, **PORTAMENTO UP/DOWN**, **TONE PORTAMENTO**, **VIBRATO**, **ATTACK/DECAY**, **SUSTAIN/RELEASE**, **WAVE**, **WAVE/PULSE/FILTER TABLE**, **FILTER CONTROL**, **FILTER CUTOFF**, **MASTER VOLUME**, **FUNK TEMPO**, **TEMPO**). Some commands are hidden when editing from the wave table.
- **CLOSE** — Closes the editor and keeps the last values.

Parameter pane (depends on the command):

- Portamento / vibrato / funk tempo — Pick a speed-table slot, or **OFF** / **TIE NOTE** / **NO CHANGE** on slot `00`. **PRECALCULATED** vs **NOTE-INDEPENDENT** plus sliders for steps/speed/shift (or even/odd rows for funk tempo).
- Attack/decay and sustain/release — ADSR nibble sliders.
- Wave — SID waveform flags (noise, pulse, saw, triangle, test, ring, sync, gate).
- Wave/pulse/filter table — Pick an **instrument** whose table to use, or **OFF**. This is not a raw table address (see [Differences from GoatTracker 2](#differences-from-goattracker-2)).
- Filter control — **VOICE 1/2/3** and resonance.
- Filter cutoff / master volume — Value sliders.
- Tempo — **THIS VOICE ONLY** vs **ALL VOICES**, plus tempo value.
```

- [ ] **Step 3: Commit** with message `Document the command editor.`

---

### Task 9: Instrument header and fields

**Files:**
- Modify: `docs/manual.markdown` `## Instrument`
- Optionally shots: `instr-header`, `instr-adsr`, `instr-vibrato`, `instr-gatetimer`, `instr-firstwave`

**Interfaces:**
- Consumes: `src/instrument_view.cpp` header + `CursorSelect::Adsr|Vibrato|GateTimer|FirstWave`

- [ ] **Step 1: Convert shots if present.**

- [ ] **Step 2: Write the instrument section start** (leave tables for Task 10; keep `## Instrument manager` as the next heading)

```markdown
## Instrument

Each instrument has ADSR, vibrato, gate timer, first wave, and up to three tables (wave, pulse, filter). Tap a field in the header to edit it in the strip below the table. Tables are **per instrument**, not a global GoatTracker table page.

<p><img src="{{ '/assets/shots/instr-header.png' | relative_url }}" alt="Instrument view"></p>

- **Index** — Current instrument number (`01`–`3F`).
- **Name** — Instrument name.
- <img class="gui-icon" src="{{ '/assets/icons/decrease.png' | relative_url }}" alt=""> / <img class="gui-icon" src="{{ '/assets/icons/increase.png' | relative_url }}" alt=""> **Previous / next instrument**
- <img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy** — Copies this instrument (including its table data) to an internal buffer.
- <img class="gui-icon" src="{{ '/assets/icons/paste.png' | relative_url }}" alt=""> **Paste** — Pastes that buffer onto the current instrument.
- **WAVE / PULSE / FILTER** — Selects which table is shown. A shaded tab means this instrument has no table of that type yet.
- <img class="gui-icon" src="{{ '/assets/icons/share.png' | relative_url }}" alt=""> **Share** — Opens table sharing. Highlighted when at least two instruments use this table.

### ADSR

<p><img src="{{ '/assets/shots/instr-adsr.png' | relative_url }}" alt="ADSR editor"></p>

Tap the `AD SR` field. Four sliders: **ATTACK**, **DECAY**, **SUSTAIN**, **RELEASE**.

### Vibrato

<p><img src="{{ '/assets/shots/instr-vibrato.png' | relative_url }}" alt="Vibrato editor"></p>

Tap the vibrato field.

- **VIBRATO DELAY** — Frames before vibrato starts.
- **STEPS** — Vibrato speed-table steps.
- **PRECALCULATED** / **NOTE-INDEPENDENT** — How speed is stored (absolute vs shift).
- **SPEED** or **SHIFT** — Depends on the mode above.

### Gate timer

<p><img src="{{ '/assets/shots/instr-gatetimer.png' | relative_url }}" alt="Gate timer editor"></p>

- **GATE TIMER** — Hard-restart / gate timing value.
- **HARD RESTART** — When off, sets the “no hard restart” flag on the gate timer byte.
- **DISABLE GATE** — When off, sets the “disable gate” flag.

### First wave

<p><img src="{{ '/assets/shots/instr-firstwave.png' | relative_url }}" alt="First wave editor"></p>

- **WAVE** — Use a SID waveform byte (then the eight waveform flag buttons).
- **GATE ON** / **GATE OFF** / **NO CHANGE** — Special first-wave values (`FE` / `FF` / `00`).
```

- [ ] **Step 3: Commit** with message `Document instrument header, ADSR, vibrato, gate, and first wave.`

---

### Task 10: Instrument tables and sharing

**Files:**
- Modify: `docs/manual.markdown` (append before `## Instrument manager`)
- Optionally shots: `instr-table-tools`, `instr-wave-wave`, `instr-wave-delay`, `instr-wave-command`, `instr-pulse-set`, `instr-pulse-mod`, `instr-filter-params`, `instr-filter-cutoff`, `instr-filter-mod`, `instr-share`

**Interfaces:**
- Consumes: `src/instrument_view.cpp` table row tools + per-table editors + share window

- [ ] **Step 1: Convert shots if present.**

- [ ] **Step 2: Append**

```markdown
### Table rows

When the table (not a header field) is selected:

<p><img src="{{ '/assets/shots/instr-table-tools.png' | relative_url }}" alt="Table row tools"></p>

- <img class="gui-icon" src="{{ '/assets/icons/deleterow.png' | relative_url }}" alt=""> **Delete row** — Removes this table row (and the whole table if it was the last data row).
- <img class="gui-icon" src="{{ '/assets/icons/addrowabove.png' | relative_url }}" alt=""> **Add row above** — Inserts a copy of this row. Disabled if the table memory is full.
- <img class="gui-icon" src="{{ '/assets/icons/addrowbelow.png' | relative_url }}" alt=""> **Add row below** — Inserts a row below, or creates the table (with a jump row) if it was empty.
- <img class="gui-icon" src="{{ '/assets/icons/jumpback.png' | relative_url }}" alt=""> **Loop here** — Toggles the table’s jump so it loops to this row (or stops looping).

Selecting a row also opens an editor for that row’s left/right bytes:

**Wave table**

<p><img src="{{ '/assets/shots/instr-wave-wave.png' | relative_url }}" alt="Wave table WAVE mode"></p>

- **WAVE / DELAY / COMMAND** — Row kind.
- WAVE: eight waveform flags (same icons as first wave). Then **RELATIVE**, **ABSOLUTE**, or **NO CHANGE** for the note column, with a slider.
- DELAY: delay slider.
- COMMAND: **EDIT COMMAND** opens the command editor for this wave-table command.

**Pulse table**

- **SET PULSE WIDTH** — Width slider.
- **MOD PULSE WIDTH** — Steps and signed speed.

**Filter table**

- **SET PARAMS** — **VOICE 1/2/3**, lowpass/bandpass/highpass icons, resonance.
- **SET CUTOFF** — Cutoff slider.
- **MOD CUTOFF** — Steps and signed speed.

### Table sharing

<p><img src="{{ '/assets/shots/instr-share.png' | relative_url }}" alt="Table sharing window"></p>

- Tap another instrument to point this instrument at that instrument’s table (and delete this table if nothing else used it).
- **CLONE** — Copies the table to a new unique block. Disabled unless the table is shared and there is room.
- **DELETE** — Detaches this instrument from the table (and deletes the bytes if this was the last user).
- **CLOSE** — Closes the window.
```

- [ ] **Step 3: Commit** with message `Document instrument tables and sharing.`

---

### Task 11: Instrument manager

**Files:**
- Modify: `docs/manual.markdown` `## Instrument manager`
- Optionally: `instr-manager-files`, `instr-manager-presets`

**Interfaces:**
- Consumes: `src/instrument_manager_view.cpp`

- [ ] **Step 1: Convert shots if present.**

- [ ] **Step 2: Write**

```markdown
## Instrument manager

Open with a second tap on **INSTR**. Load and save `.ins` files for the **current** piano instrument.

<p><img src="{{ '/assets/shots/instr-manager-files.png' | relative_url }}" alt="Instrument manager files"></p>

- **FILES** — User instrument files.
- **PRESETS** — Bundled presets.
- Name field (FILES) — File name for save.
- **LOAD** — Loads the selected file or preset into the current instrument.
- **SAVE** — Saves the current instrument. Confirms overwrite if the name exists.
- **DELETE** — Deletes the selected user file after confirm.
```

- [ ] **Step 3: Commit** with message `Document the instrument manager.`

---

### Task 12: Settings

**Files:**
- Modify: `docs/manual.markdown` `## Settings`
- Optionally: `settings-project`, `settings-editor`, `settings-hardrestart`, `settings-sampling`

**Interfaces:**
- Consumes: `src/settings_view.cpp`

- [ ] **Step 1: Convert shots if present.**

- [ ] **Step 2: Write**

```markdown
## Settings

### Project settings

<p><img src="{{ '/assets/shots/settings-project.png' | relative_url }}" alt="Project settings"></p>

- **CHIP MODEL** — **6581** or **8580**.
- **SPEED** — Multiplier (`25Hz` when 0, otherwise `N`×). Also updates gate timer on unused instruments.
- **HARD RESTART** — Opens the hard-restart ADSR window (`adparam`).

<p><img src="{{ '/assets/shots/settings-hardrestart.png' | relative_url }}" alt="Hard restart window"></p>

- Attack/decay/sustain/release sliders for the hard-restart parameter.
- **CLOSE** — Closes the window.

### Editor settings

<p><img src="{{ '/assets/shots/settings-editor.png' | relative_url }}" alt="Editor settings"></p>

- **FULLSCREEN** — Toggle (Android).
- **KEEP SCREEN ON** — Toggle.
- **ROW HEIGHT** / **HIGHLIGHT** — Pattern row size and highlight spacing.
- **REG WRITE ORDER** — SID register write order **v2.68** or **v2.73**.
- **SAMPLING METHOD** — Opens the sampling-method window.

<p><img src="{{ '/assets/shots/settings-sampling.png' | relative_url }}" alt="Sampling method window"></p>

- **FAST**, **INTERPOLATE**, **RESAMPLE INTERPOLATE**, **RESAMPLE FAST** — Picks reSID sampling.
- **CLOSE** — Closes without changing more than the last tap.
```

- [ ] **Step 3: Commit** with message `Document project and editor settings.`

---

### Task 13: Differences from GoatTracker 2

**Files:**
- Modify: `docs/manual.markdown` `## Differences from GoatTracker 2`
- Modify: `todo.md`

**Interfaces:**
- Consumes: spec pass-2 seed; `src/gtsong.hpp` `STBL_*`; `src/command_edit.cpp` table-pointer instrument list; `src/instrument_view.cpp` sharing; `src/project_view.cpp` export

- [ ] **Step 1: Verify claims in source** (read, do not change app code)

Confirm still true:

- No global WAVE/PULSE/FILTER table view — only `instrument_view` tabs.
- `STBL_PORTA_START`, `STBL_VIB_START`, `STBL_FUNK_START` in `src/gtsong.hpp`.
- Commands 8/9/A iterate `g_song.instruments[r]` in `command_edit.cpp`.
- Export formats include SID in `project_view.cpp`.

If a claim is wrong, drop or correct it in the markdown; do not document fiction.

- [ ] **Step 2: Write the chapter**

```markdown
## Differences from GoatTracker 2

GTMobile songs are still `.sng` and mostly play in GoatTracker 2, but the editor is not a straight port of the desktop UI.

**No global table view.** Wave, pulse, and filter tables are edited on the instrument that uses them. There is no packed “all tables” page with raw row addresses.

**Speed table.** GoatTracker’s speed table is split internally into portamento, vibrato, and funk-tempo ranges. You never edit STBL as one list. Instrument vibrato uses the vibrato range; pattern commands pick a slot in the matching sub-list (or OFF / TIE NOTE / NO CHANGE).

**Table pointer commands (`8` / `9` / `A`).** In GoatTracker these store a table **address**. In GTMobile the data byte is an **instrument number** (or `00` for off). Playback uses that instrument’s wave, pulse, or filter pointer. Long-press **Edit command** on such a row to jump to that instrument and table.

**Table sharing.** Several instruments can point at the same table. **Share** clones or deletes that link instead of compacting a global table by hand.

**Order list.** Loop start is a row mark (**Loop here**). Transpose lives in the pattern index window. **Clone pattern** copies pattern data to a free slot.

**Playback and entry.** Follow, pattern loop, record, auto-step, and the on-screen piano have no GoatTracker-menu equivalent. There is no GT2 keyboard map.

**Export.** From Project you can export **SNG**, **SID**, **WAV**, and **OGG**. You do not need a separate desktop conversion step for SID.
```

- [ ] **Step 3: Update `todo.md`**

Change the website/documentation block to:

```markdown
# website/documentation
+ [ ] record video
+ [x] manual
+ [ ] tutorial
+ [x] differences to GoatTracker2
```

Remove the nested unchecked difference bullets (they are covered by the chapter).

- [ ] **Step 4: Commit** with message `Document differences from GoatTracker 2.`

---

### Task 14: Jekyll sanity check

**Files:** none required unless the check finds leftover TODOs or broken relative paths.

- [ ] **Step 1: Leftover TODO / old asset names**

```bash
grep -nE 'TODO|export\.png|song-cell-buttons|pattern-cell-buttons|song2\.png' docs/manual.markdown || true
```

Expected: no matches (unless you still use a file that exists). `docs/assets/project.png` and `docs/assets/song2.png` may remain on disk unused; do not delete unless you are sure nothing else links them. The manual must not link missing images.

- [ ] **Step 2: Every `<img src>` file exists**

From repo root:

```bash
python3 - <<'PY'
from pathlib import Path
import re
text = Path("docs/manual.markdown").read_text()
# Jekyll: {{ '/assets/foo' | relative_url }}
paths = re.findall(r"\{\{\s*'/([^']+)'\s*\|\s*relative_url\s*\}\}", text)
missing = [p for p in paths if not (Path("docs") / p).is_file()]
print("refs", len(paths))
print("missing", missing)
raise SystemExit(1 if missing else 0)
PY
```

Expected: exit 0. If missing, either add the PNG or remove that `<img>` tag, then re-run.

- [ ] **Step 3: Jekyll build**

```bash
cd docs && bundle exec jekyll build
```

Expected: success. If bundler gems are missing, `bundle install` then retry. If Ruby/Jekyll is unavailable, skip the build and rely on Step 2; note that in the commit message body only if you cannot build.

- [ ] **Step 4: Commit** only if Step 2/3 caused edits, message `Fix manual image refs after Jekyll check.` If there is nothing to commit, stop.

---

## Coverage (spec)

| Spec item | Task |
|---|---|
| Icon script + PNGs + CSS | 1–2 |
| Intro, no video | 2 |
| Always-on, piano, project, song, command, instrument, manager, settings | 3–12 |
| Shot ids / convert-if-present | 3–12 |
| Differences + todo.md | 13 |
| No broken images / Jekyll | 14 |
| SID export documented | 5, 13 |
