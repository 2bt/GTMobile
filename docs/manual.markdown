---
layout: page
title: Manual
permalink: /manual/
---

GTMobile is a C64 SID tracker for Android. It is based on and largely compatible with [GoatTracker 2](https://sourceforge.net/projects/goattracker2/). You edit one song at a time.

This page is a control reference: each screen (and each selection mode) is listed with what the buttons do. GoatTracker users should also read [Differences from GoatTracker 2](#differences-from-goattracker-2).

Tap the splash screen to start. After that the layout is always: tabs and undo/redo at the top, the current view in the middle, the piano above the playback buttons at the bottom.

## Always-on controls

These controls stay on screen in every view.

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

## Piano

The piano is available in every view. It plays the current instrument through the SID emulator.

- **Instrument** (left, shows index and name) — Opens the instrument picker. In **Song** view, long-press this button to set the piano instrument from the selected pattern row (if that row has an instrument).
- **Octave bar** — Drag horizontally to scroll which keys are visible.

**Instrument picker:**

- Tap an instrument to select it and close the picker. Empty instruments (no table pointers) are shaded.
- **CLOSE** — Closes the picker without changing the instrument.

## Project

Song metadata and files. GTMobile edits one song at a time.

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

**DEMOS toolbar**

- **LOAD** — Loads the selected demo after confirm.

**Export window**

- Format row — **SNG**, **SID**, **WAV**, or **OGG**.
- **EXPORT** — Writes that format and hands the file to the system share/export flow. WAV and OGG render in the background with a progress bar.
- **CLOSE** — Closes the window.
- **CANCEL** — Aborts an in-progress WAV/OGG render.

**Confirm / alert** (used by load, save overwrite, delete, reset, import, and errors)

- **OK** — Accepts the action (or dismisses an alert).
- **CANCEL** — Dismisses without changing the song (confirm only).

## Song

A song is an **order list** (which patterns play on each of the three SID voices) plus the **patterns** themselves. Instruments are numbered `01`–`3F`. Patterns are numbered `00`–`CF`.

**Tap** an order-list cell to select it (that row’s three patterns appear below). **Long-press and drag** to select a rectangle of cells.

### Order list — one cell

- <img class="gui-icon" src="{{ '/assets/icons/paste.png' | relative_url }}" alt=""> **Paste** — Pastes a copied order-list region starting at this cell.
- <img class="gui-icon" src="{{ '/assets/icons/deleterow.png' | relative_url }}" alt=""> **Delete row** — Deletes this order row on all voices. Disabled if only one row remains.
- <img class="gui-icon" src="{{ '/assets/icons/addrowabove.png' | relative_url }}" alt=""> **Add row above** — Inserts a copy of this row above. Disabled if the order list is full.
- <img class="gui-icon" src="{{ '/assets/icons/addrowbelow.png' | relative_url }}" alt=""> **Add row below** — Inserts a copy of this row below.
- <img class="gui-icon" src="{{ '/assets/icons/jumpback.png' | relative_url }}" alt=""> **Loop here** — Marks this row as the loop start when the song repeats. The row shows a small loop mark.
- <img class="gui-icon" src="{{ '/assets/icons/clone.png' | relative_url }}" alt=""> **Clone pattern** — Copies this cell’s pattern into a free pattern slot and points the cell at the copy (only if the source pattern is not empty).
- <img class="gui-icon" src="{{ '/assets/icons/edit.png' | relative_url }}" alt=""> **Edit** — Opens the pattern index window for this cell.

### Order list — region

- <img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy** — Copies the selected order cells.
- <img class="gui-icon" src="{{ '/assets/icons/edit.png' | relative_url }}" alt=""> **Edit** — Opens the pattern index window for every cell in the selection (same as Edit above).

### Pattern index window

- Tap a pattern number to assign it to the selected cell(s). Empty patterns are shaded. Long-press a number to drag-reorder pattern slots.
- **TRANSPOSE** slider — Sets order-list transpose for the selected cell(s) (`-F` … `+E`).
- **CLOSE** — Closes the window.

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

- **LENGTH** slider — Sets this pattern’s row count.
- **RESIZE EMPTY PATTERNS** — Sets every empty pattern to this length.
- **SHRINK** — Drops every other row (halves length). Disabled at length 1.
- **EXPAND** — Inserts a blank row after each row (doubles length). Disabled if that would exceed the maximum.
- **CLOSE** — Closes and clears unused rows beyond the new length.

### Auto-step window

- **STEP** slider — How many rows to advance on note entry (1–8).
- **CLOSE** — Closes the window.

### Pattern — region

- <img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy** — Copies notes and commands.
- <img class="gui-icon" src="{{ '/assets/icons/x.png' | relative_url }}" alt=""> **Clear** — Clears notes and commands in the region.
- <img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy notes** — Copies only notes and instruments.
- <img class="gui-icon" src="{{ '/assets/icons/x.png' | relative_url }}" alt=""> **Clear notes** — Clears notes and instruments.
- **Transpose up** / **Transpose down** — Moves notes in the region by one semitone (clamped to the valid note range).
- <img class="gui-icon" src="{{ '/assets/icons/piano.png' | relative_url }}" alt=""> **Set instrument** — Sets every existing instrument index in the region to the piano’s current instrument.
- <img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy commands** — Copies only commands.
- <img class="gui-icon" src="{{ '/assets/icons/x.png' | relative_url }}" alt=""> **Clear commands** — Clears commands and data.
- <img class="gui-icon" src="{{ '/assets/icons/edit.png' | relative_url }}" alt=""> **Edit command** — Opens the command editor and applies the result to every row in the region.

## Command editor

Opens from a pattern row, a pattern region, or a wave-table command row. Left: command list. Right: parameters for the selected command. Values are stored with the song; this window edits them.

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

## Instrument

Each instrument has ADSR, vibrato, gate timer, first wave, and up to three tables (wave, pulse, filter). Tap a field in the header to edit it in the strip below the table. Tables are **per instrument**, not a global GoatTracker table page.

- **Index** — Current instrument number (`01`–`3F`).
- **Name** — Instrument name.
- <img class="gui-icon" src="{{ '/assets/icons/decrease.png' | relative_url }}" alt=""> / <img class="gui-icon" src="{{ '/assets/icons/increase.png' | relative_url }}" alt=""> **Previous / next instrument**
- <img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy** — Copies this instrument (including its table data) to an internal buffer.
- <img class="gui-icon" src="{{ '/assets/icons/paste.png' | relative_url }}" alt=""> **Paste** — Pastes that buffer onto the current instrument.
- **WAVE / PULSE / FILTER** — Selects which table is shown. A shaded tab means this instrument has no table of that type yet.
- <img class="gui-icon" src="{{ '/assets/icons/share.png' | relative_url }}" alt=""> **Share** — Opens table sharing. Highlighted when at least two instruments use this table.

### ADSR

Tap the `AD SR` field. Four sliders: **ATTACK**, **DECAY**, **SUSTAIN**, **RELEASE**.

### Vibrato

Tap the vibrato field.

- **VIBRATO DELAY** — Frames before vibrato starts.
- **STEPS** — Vibrato speed-table steps.
- **PRECALCULATED** / **NOTE-INDEPENDENT** — How speed is stored (absolute vs shift).
- **SPEED** or **SHIFT** — Depends on the mode above.

### Gate timer

- **GATE TIMER** — Hard-restart / gate timing value.
- **HARD RESTART** — When off, sets the “no hard restart” flag on the gate timer byte.
- **DISABLE GATE** — When off, sets the “disable gate” flag.

### First wave

- **WAVE** — Use a SID waveform byte (then the eight waveform flag buttons).
- **GATE ON** / **GATE OFF** / **NO CHANGE** — Special first-wave values (`FE` / `FF` / `00`).

### Table rows

When the table (not a header field) is selected:

- <img class="gui-icon" src="{{ '/assets/icons/deleterow.png' | relative_url }}" alt=""> **Delete row** — Removes this table row (and the whole table if it was the last data row).
- <img class="gui-icon" src="{{ '/assets/icons/addrowabove.png' | relative_url }}" alt=""> **Add row above** — Inserts a copy of this row. Disabled if the table memory is full.
- <img class="gui-icon" src="{{ '/assets/icons/addrowbelow.png' | relative_url }}" alt=""> **Add row below** — Inserts a row below, or creates the table (with a jump row) if it was empty.
- <img class="gui-icon" src="{{ '/assets/icons/jumpback.png' | relative_url }}" alt=""> **Loop here** — Toggles the table’s jump so it loops to this row (or stops looping).

Selecting a row also opens an editor for that row’s left/right bytes:

**Wave table**

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

- Tap another instrument to point this instrument at that instrument’s table (and delete this table if nothing else used it).
- **CLONE** — Copies the table to a new unique block. Disabled unless the table is shared and there is room.
- **DELETE** — Detaches this instrument from the table (and deletes the bytes if this was the last user).
- **CLOSE** — Closes the window.

## Instrument manager

Open with a second tap on **INSTR**. Load and save `.ins` files for the **current** piano instrument.

- **FILES** — User instrument files.
- **PRESETS** — Bundled presets.
- Name field (FILES) — File name for save.
- **LOAD** — Loads the selected file or preset into the current instrument.
- **SAVE** — Saves the current instrument. Confirms overwrite if the name exists.
- **DELETE** — Deletes the selected user file after confirm.

## Settings

### Project settings

- **CHIP MODEL** — **6581** or **8580**.
- **SPEED** — Multiplier (`25Hz` when 0, otherwise `N`×). Also updates gate timer on unused instruments.
- **HARD RESTART** — Opens the hard-restart ADSR window (`adparam`).

Hard-restart window:

- Attack/decay/sustain/release sliders for the hard-restart parameter.
- **CLOSE** — Closes the window.

### Editor settings

- **FULLSCREEN** — Toggle (Android).
- **KEEP SCREEN ON** — Toggle.
- **ROW HEIGHT** / **HIGHLIGHT** — Pattern row size and highlight spacing.
- **REG WRITE ORDER** — SID register write order **v2.68** or **v2.73**.
- **SAMPLING METHOD** — Opens the sampling-method window.

Sampling-method window:

- **FAST**, **INTERPOLATE**, **RESAMPLE INTERPOLATE**, **RESAMPLE FAST** — Picks reSID sampling.
- **CLOSE** — Closes without changing more than the last tap.

## Differences from GoatTracker 2

GTMobile songs are still `.sng` and mostly play in GoatTracker 2, but the editor is not a straight port of the desktop UI.

**No global table view.** Wave, pulse, and filter tables are edited on the instrument that uses them. There is no packed “all tables” page with raw row addresses.

**Speed table.** GoatTracker’s speed table is split internally into portamento, vibrato, and funk-tempo ranges. You never edit STBL as one list. Instrument vibrato uses the vibrato range; pattern commands pick a slot in the matching sub-list (or OFF / TIE NOTE / NO CHANGE).

**Table pointer commands (`8` / `9` / `A`).** In GoatTracker these store a table **address**. In GTMobile the data byte is an **instrument number** (or `00` for off). Playback uses that instrument’s wave, pulse, or filter pointer. Long-press **Edit command** on such a row to jump to that instrument and table.

**Table sharing.** Several instruments can point at the same table. **Share** clones or deletes that link instead of compacting a global table by hand.

**Order list.** Loop start is a row mark (**Loop here**). Transpose lives in the pattern index window. **Clone pattern** copies pattern data to a free slot.

**Playback and entry.** Follow, pattern loop, record, auto-step, and the on-screen piano have no GoatTracker-menu equivalent. There is no GT2 keyboard map.

**Export.** From Project you can export **SNG**, **SID**, **WAV**, and **OGG**. You do not need a separate desktop conversion step for SID.
