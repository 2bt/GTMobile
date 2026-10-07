---
layout: page
title: Manual
permalink: /manual/
---

GTMobile is a C64 SID tracker for Android. It is based on and largely compatible with [GoatTracker 2](https://sourceforge.net/projects/goattracker2/).

This page describes the controls, in the order they appear in the app. Always-on controls come first, then the project, song, instrument, and settings views. Each section shows the screen and lists what its buttons and gestures do. The last section covers where a GTMobile song differs from a GoatTracker 2 song.

- [Always-On Controls](#always-on-controls)
- [Project View](#project-view)
- [Song View](#song-view)
- [Command Editor](#command-editor)
- [Instrument View](#instrument-view)
- [Instrument Manager View](#instrument-manager-view)
- [Settings View](#settings-view)
- [Differences from GoatTracker 2](#differences-from-goattracker-2)

## Always-On Controls

These controls stay on screen in every view: tabs and undo/redo at the top, the piano and playback buttons at the bottom.

<p><img src="{{ '/assets/shots/always-on.png' | relative_url }}" alt="Tabs, undo, and playback"></p>

### Tabs

- **PROJECT** — Opens the [project view](#project-view).
- **SONG** — Opens the [song view](#song-view).
- **INSTR** — Opens the [instrument editor](#instrument-view). Tap again to open the [instrument manager](#instrument-manager-view).
- <img class="gui-icon" src="{{ '/assets/icons/settings.png' | relative_url }}" alt=""> **Settings** — Opens the [settings view](#settings-view).
- <img class="gui-icon" src="{{ '/assets/icons/undo.png' | relative_url }}" alt=""> **Undo** — Reverts the last song edit.
- <img class="gui-icon" src="{{ '/assets/icons/redo.png' | relative_url }}" alt=""> **Redo** — Re-applies an undone edit.

### Piano

The piano plays the current instrument on the voice selected in the song view.

- **Instrument** (left, shows index and name) — Opens the instrument picker. In the song view, long-press this button to set the piano instrument from the selected pattern row (if that row has an instrument).
- **Octave scrollbar** — Drag horizontally to scroll which keys are visible.

#### Instrument Picker

<p><img src="{{ '/assets/shots/piano-select.png' | relative_url }}" alt="Instrument picker"></p>

Instruments are numbered `01`–`3F`.

- Tap an instrument to select it and close the picker. Empty instruments (no table pointers) are shaded. Long-press an instrument and drag to reorder instrument slots.
- **CLOSE** — Closes the window.

### Playback

- <img class="gui-icon" src="{{ '/assets/icons/fastbackward.png' | relative_url }}" alt=""> **Fast Backward** — Jumps to the start of the current pattern. Tap again to go back one order-list row.
- <img class="gui-icon" src="{{ '/assets/icons/stop.png' | relative_url }}" alt=""> **Stop** — Stops playback and sets the restart point to the current order row.
- <img class="gui-icon" src="{{ '/assets/icons/playpause.png' | relative_url }}" alt=""> **Play/Pause** — Starts playback from the current start position, or pauses if already playing.
- <img class="gui-icon" src="{{ '/assets/icons/follow.png' | relative_url }}" alt=""> **Follow** — When on, the pattern view follows playback.
- <img class="gui-icon" src="{{ '/assets/icons/loop.png' | relative_url }}" alt=""> **Loop** — When on, playback loops the current pattern instead of advancing in the order list.
- <img class="gui-icon" src="{{ '/assets/icons/fastforward.png' | relative_url }}" alt=""> **Fast Forward** — Skips to the next order-list row.

## Project View

The project view is where you name the song and work with song files.

<p><img src="{{ '/assets/shots/project-files.png' | relative_url }}" alt="Project files tab"></p>

- **TITLE** — Song title, stored in the song file and in an exported SID.
- **AUTHOR** — Author name.
- **RELEASED** — Copyright / release note.
- **FILES** — Songs saved on the device.
- **DEMOS** — Bundled demo songs. They can only be loaded.
- **File name** (FILES) — Name of the current song file. LOAD, SAVE, DELETE, and EXPORT use this name.
- **LOAD** — Loads the selected file or demo.
- **SAVE** — Saves under the file name.
- **DELETE** — Deletes the selected file.
- **IMPORT** — Imports a song from the system file picker.
- **EXPORT** — Opens the export window. Disabled until the file name is set.
- **RESET** — Clears the song.

### Export Window

<p><img src="{{ '/assets/shots/project-export.png' | relative_url }}" alt="Export window"></p>

- **SNG** / **SID** / **WAV** / **OGG** — Export format.
- **EXPORT** — Writes that format and hands the file to the system share/export flow. WAV and OGG render in the background with a progress bar.
- **CLOSE** — Closes the window.
- **CANCEL** — Aborts an in-progress WAV/OGG render.

## Song View

The song view has an order list and patterns. Each order-list row says which pattern plays on each of the three SID voices.

**Tap** an order-list cell to select it (that row’s three patterns appear below). **Long-press and drag** to select a region of cells.

### Order List — One Cell

<p><img src="{{ '/assets/shots/song-order-cell.png' | relative_url }}" alt="Order list single cell buttons"></p>

- <img class="gui-icon" src="{{ '/assets/icons/paste.png' | relative_url }}" alt=""> **Paste** — Pastes a copied order-list region starting at this cell.
- <img class="gui-icon" src="{{ '/assets/icons/deleterow.png' | relative_url }}" alt=""> **Delete Row** — Deletes this order row on all voices.
- <img class="gui-icon" src="{{ '/assets/icons/addrowabove.png' | relative_url }}" alt=""> **Add Row Above** — Inserts a copy of this row above.
- <img class="gui-icon" src="{{ '/assets/icons/addrowbelow.png' | relative_url }}" alt=""> **Add Row Below** — Inserts a copy of this row below.
- <img class="gui-icon" src="{{ '/assets/icons/jumpback.png' | relative_url }}" alt=""> **Loop Here** — Marks this row as the loop start when the song repeats. The row shows a small loop mark.
- <img class="gui-icon" src="{{ '/assets/icons/clone.png' | relative_url }}" alt=""> **Clone Pattern** — Copies this cell’s pattern into a free pattern slot and points the cell at the copy (only if the source pattern is not empty).
- <img class="gui-icon" src="{{ '/assets/icons/edit.png' | relative_url }}" alt=""> **Edit** — Opens the pattern index window for this cell.

### Order List — Region

<p><img src="{{ '/assets/shots/song-order-region.png' | relative_url }}" alt="Order list region buttons"></p>

- <img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy** — Copies the selected order cells.
- <img class="gui-icon" src="{{ '/assets/icons/edit.png' | relative_url }}" alt=""> **Edit** — Same as [Edit](#order-list--one-cell), for every cell in the selection.

### Pattern Index Window

<p><img src="{{ '/assets/shots/song-order-edit.png' | relative_url }}" alt="Pattern index window"></p>

Patterns are numbered `00`–`CF`.

- Tap a pattern number to assign it to the selected cell(s). Empty patterns are shaded. Long-press a number and drag to reorder pattern slots.
- **TRANSPOSE** slider — Sets order-list transpose for the selected cell(s).
- **CLOSE** — Closes the window.

### Pattern Channels

Above the pattern rows, three buttons show the current pattern index per voice. Tap one to mute or unmute that voice. The small bar is the SID envelope level. Drag the handle on the right (between the two scroll bars) to move this split up or down.

### Pattern — One Row

<p><img src="{{ '/assets/shots/song-pattern-cell.png' | relative_url }}" alt="Pattern single row buttons"></p>

**Tap** a pattern row to select it. **Long-press and drag** to select a region.

- <img class="gui-icon" src="{{ '/assets/icons/paste.png' | relative_url }}" alt=""> **Paste** — Same as [Paste](#order-list--one-cell), for notes and commands.
- <img class="gui-icon" src="{{ '/assets/icons/changelength.png' | relative_url }}" alt=""> **Pattern Length** — Opens the pattern length window.
- <img class="gui-icon" src="{{ '/assets/icons/deleterow.png' | relative_url }}" alt=""> **Delete Row** — Shifts later rows up and clears the last row (does not change pattern length).
- <img class="gui-icon" src="{{ '/assets/icons/addrowabove.png' | relative_url }}" alt=""> **Add Row Above** — Shifts this row and below down, leaving an empty row here.

#### Note Tools

- <img class="gui-icon" src="{{ '/assets/icons/x.png' | relative_url }}" alt=""> **Clear Note** — Clears the note and instrument on this row (auto-steps if auto-step is on).
- **Gate off/on** — Writes a gate-off. Tap again on a gate-off row to write gate-on.
- <img class="gui-icon" src="{{ '/assets/icons/record.png' | relative_url }}" alt=""> **Record** — When on, piano keys write the current instrument’s note into this row (and auto-step).
- <img class="gui-icon" src="{{ '/assets/icons/autostep.png' | relative_url }}" alt=""> **Auto-Step** — When on, note entry advances the cursor. Long-press opens the auto-step window.

#### Command Tools

- <img class="gui-icon" src="{{ '/assets/icons/x.png' | relative_url }}" alt=""> **Clear Command** — Clears the pattern command and data.
- <img class="gui-icon" src="{{ '/assets/icons/edit.png' | relative_url }}" alt=""> **Edit Command** — Opens the [command editor](#command-editor). If the command is a table pointer (wave, pulse, or filter table), long-press jumps to that instrument and table in the instrument view.

#### Playback From Cursor

- <img class="gui-icon" src="{{ '/assets/icons/play.png' | relative_url }}" alt=""> **Play From Here** — Starts the song at this order row and pattern row.
- <img class="gui-icon" src="{{ '/assets/icons/playrow.png' | relative_url }}" alt=""> **Play Row** — Plays this pattern row once (and auto-steps).

### Pattern Length Window

<p><img src="{{ '/assets/shots/song-pattern-length.png' | relative_url }}" alt="Pattern length window"></p>

- **LENGTH** slider — Sets this pattern’s row count.
- **RESIZE EMPTY PATTERNS** — Sets every empty pattern to this length.
- **SHRINK** — Drops every other row (halves length).
- **EXPAND** — Inserts a blank row after each row (doubles length).
- **CLOSE** — Closes the window.

### Auto-Step Window

<p><img src="{{ '/assets/shots/song-autostep.png' | relative_url }}" alt="Auto-step window"></p>

- **STEP** slider — How many rows to advance on note entry (1–8).
- **CLOSE** — Closes the window.

### Pattern — Region

<p><img src="{{ '/assets/shots/song-pattern-region.png' | relative_url }}" alt="Pattern region buttons"></p>

- <img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy** — Same as [Copy](#order-list--region), for notes and commands.
- <img class="gui-icon" src="{{ '/assets/icons/x.png' | relative_url }}" alt=""> **Clear** — Clears notes and commands in the region.
- <img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy Notes** — Copies only notes and instruments.
- <img class="gui-icon" src="{{ '/assets/icons/x.png' | relative_url }}" alt=""> **Clear Notes** — Clears notes and instruments.
- **Transpose up** / **Transpose down** — Moves notes in the region by one semitone (clamped to the valid note range).
- <img class="gui-icon" src="{{ '/assets/icons/piano.png' | relative_url }}" alt=""> **Set Instrument** — Sets every existing instrument index in the region to the piano’s current instrument.
- <img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy Commands** — Copies only commands.
- <img class="gui-icon" src="{{ '/assets/icons/x.png' | relative_url }}" alt=""> **Clear Commands** — Clears commands and data.
- <img class="gui-icon" src="{{ '/assets/icons/edit.png' | relative_url }}" alt=""> **Edit Command** — Opens the command editor and applies the result to every row in the region.

## Command Editor

The command editor opens from a pattern row, a pattern region, or a wave-table command row. Commands are listed on the left. Parameters for the selected command appear on the right. Parameter changes are written as you edit.

- Tap a command name to select it.
- **CLOSE** — Closes the window.

Some commands are hidden when editing from the wave table.

### Portamento

<p><img src="{{ '/assets/shots/song-command-porta.png' | relative_url }}" alt="Portamento command"></p>

**PORTAMENTO UP**, **PORTAMENTO DOWN**, and **TONE PORTAMENTO** share this layout. Pick a speed-table slot. Slot `00` is **OFF**, or **TIE NOTE** for tone portamento.

- **PRECALCULATED** — Adds or subtracts a fixed amount from the voice frequency each tick. Fine control, but the same amount covers more semitones on low notes than on high notes. **HI** and **LO** set that 16-bit amount.
- **NOTE-INDEPENDENT** — Takes the current note’s pitch, shifts it right, and uses the result as the step, so the rate stays similar on every note. Coarser than precalculated. **SHIFT** is how many times to shift. Higher is slower.

### Vibrato

Same window as [portamento](#portamento): a speed-table slot list, then **PRECALCULATED** or **NOTE-INDEPENDENT**. Slot `00` is **OFF**.

- **VIBRATO STEPS** — How many steps the pitch travels before turning around. Higher is a wider wobble and a slower cycle.
- **SPEED** — Size of each step in precalculated mode. Higher is a wider wobble at the same cycle time.
- **SHIFT** — Size of each step in note-independent mode. Higher is a smaller step.

### Attack/Decay & Sustain/Release

<p><img src="{{ '/assets/shots/song-command-ad.png' | relative_url }}" alt="Attack/decay command"></p>

**ATTACK/DECAY** sets the attack and decay nibbles. **SUSTAIN/RELEASE** is the same layout for the other two.

### Wave

<p><img src="{{ '/assets/shots/song-command-wave.png' | relative_url }}" alt="Wave command"></p>

Same eight SID flags as the [wave table](#wave-table): **Noise**, **Pulse**, **Saw**, **Triangle**, **Test**, **Ring**, **Sync**, **Gate**.

### Table Pointer

<p><img src="{{ '/assets/shots/song-command-table.png' | relative_url }}" alt="Table pointer command"></p>

**WAVE TABLE**, **PULSE TABLE**, and **FILTER TABLE** pick an instrument whose table to use, or **OFF**. This is not a raw table address (see [Differences from GoatTracker 2](#differences-from-goattracker-2)).

### Filter Control

<p><img src="{{ '/assets/shots/song-command-filter-control.png' | relative_url }}" alt="Filter control command"></p>

- **VOICE 1** / **VOICE 2** / **VOICE 3** — Which voices go through the filter. These are the chip voices, as in the [filter table](#filter-table).
- **RES** — Filter resonance.

### Filter Cutoff

<p><img src="{{ '/assets/shots/song-command-filter-cutoff.png' | relative_url }}" alt="Filter cutoff command"></p>

Slider for the filter cutoff (`00`–`FF`).

### Master Volume

<p><img src="{{ '/assets/shots/song-command-volume.png' | relative_url }}" alt="Master volume command"></p>

Slider for the SID master volume (`0`–`F`).

### Funk Tempo

<p><img src="{{ '/assets/shots/song-command-funk.png' | relative_url }}" alt="Funk tempo command"></p>

Pick a speed-table slot. Slot `00` is **NO CHANGE**.

- **EVEN ROW** / **ODD ROW** — Ticks per pattern row, alternating even and odd rows. Lower is faster.

### Tempo

<p><img src="{{ '/assets/shots/song-command-tempo.png' | relative_url }}" alt="Tempo command"></p>

Same unit as funk tempo: ticks per pattern row. Lower is faster.

- **THIS VOICE ONLY** — Sets tempo only on the voice that runs the command.
- **ALL VOICES** — Sets the same tempo on all three voices.

## Instrument View

Instruments define how a note sounds: volume envelope (ADSR), vibrato, gate timing / hard restart, the first-frame wave, and wave, pulse, and filter tables. Pattern notes pick an instrument by number.

Tap a field above the table to edit it below. Tap a table row to edit that row.

### Header

<p><img src="{{ '/assets/shots/instr-adsr.png' | relative_url }}" alt="Instrument view with ADSR selected"></p>

- **Index** — Instrument number (`01`–`3F`). Display only.
- **Name** — Instrument name.
- <img class="gui-icon" src="{{ '/assets/icons/decrease.png' | relative_url }}" alt=""> / <img class="gui-icon" src="{{ '/assets/icons/increase.png' | relative_url }}" alt=""> **Previous / Next** — Selects the previous or next instrument.
- <img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy** — Copies this instrument, including its table data.
- <img class="gui-icon" src="{{ '/assets/icons/paste.png' | relative_url }}" alt=""> **Paste** — Pastes that copy onto the current instrument.
- **WAVE / PULSE / FILTER** — Selects which table is shown. A shaded tab means this instrument has no table of that type yet.
- <img class="gui-icon" src="{{ '/assets/icons/share.png' | relative_url }}" alt=""> **Share** — Opens the table sharing window. Highlighted when at least two instruments use this table.

#### ADSR

Tap the `ADSR` field. Four sliders: **ATTACK**, **DECAY**, **SUSTAIN**, **RELEASE**.

#### Vibrato

<p><img src="{{ '/assets/shots/instr-vibrato.png' | relative_url }}" alt="Instrument vibrato"></p>

Tap the vibrato field.

- **VIBRATO DELAY** — Frames before vibrato starts. `00` disables vibrato.
- **STEPS**, **PRECALCULATED** / **NOTE-INDEPENDENT**, **SPEED**, and **SHIFT** — Same meaning as vibrato in the [command editor](#vibrato).

#### Gate Timer

<p><img src="{{ '/assets/shots/instr-gatetimer.png' | relative_url }}" alt="Instrument gate timer"></p>

These control the ticks just before a new note starts (gate-off and hard restart). The timer must stay below the current tempo (at most tempo−1). Too high stops playback.

- **GATE TIMER** — How many ticks before the note that gate-off / hard restart run.
- **HARD RESTART** — When on, the voice is hard-restarted before the note so it retriggers cleanly (ADSR for that comes from Settings → **HARD RESTART**). When off, skip hard restart.
- **DISABLE GATE** — When off, skip the automatic gate-off before the note. With First Wave **NO CHANGE**, that is the usual legato setup (tables and ADSR still re-init).

#### First Wave

<p><img src="{{ '/assets/shots/instr-firstwave.png' | relative_url }}" alt="Instrument first wave"></p>

Waveform written on the note’s init frame. Common choice is gate + test.

- **WAVE** — Sets that init waveform with the same eight flag buttons as the [wave table](#wave-table).
- **GATE ON** — Leave the waveform unchanged, force gate on.
- **GATE OFF** — Leave the waveform unchanged, force gate off.
- **NO CHANGE** — Leave waveform and gate unchanged. With **DISABLE GATE** off, the usual legato setup (tables and ADSR still re-init).

### Instrument Tables

Each instrument can have a wave, pulse, and filter table. Switch between them with the **WAVE / PULSE / FILTER** tabs. You edit each table on the instrument that uses it (see [Differences from GoatTracker 2](#differences-from-goattracker-2)). Instruments can still share the same table.

Tables change the sound over time while a note plays. The table is processed row by row, updating wave, pulse, or filter from tick to tick.

With a table row selected, these general table tools are available:

- <img class="gui-icon" src="{{ '/assets/icons/deleterow.png' | relative_url }}" alt=""> **Delete Row** — Removes this table row (and the whole table if it was the last data row).
- <img class="gui-icon" src="{{ '/assets/icons/addrowabove.png' | relative_url }}" alt=""> **Add Row Above** — Inserts a copy of this row. Disabled if the table memory is full.
- <img class="gui-icon" src="{{ '/assets/icons/addrowbelow.png' | relative_url }}" alt=""> **Add Row Below** — Inserts a row below, or creates the table (with a jump row) if it was empty.
- <img class="gui-icon" src="{{ '/assets/icons/jumpback.png' | relative_url }}" alt=""> **Loop Here** — Toggles the table’s jump so it loops to this row (or stops looping).

Selecting a row also shows its controls below.

#### Wave Table

Controls waveform and pitch over time.

<p><img src="{{ '/assets/shots/instr-wave-wave.png' | relative_url }}" alt="Wave table row in WAVE mode"></p>

- **WAVE** — Sets the SID waveform control register with these eight flags. Without **Noise**, **Pulse**, **Saw**, or **Triangle**, there is no sound.
  - <img class="gui-icon" src="{{ '/assets/icons/noise.png' | relative_url }}" alt=""> **Noise** — LFSR noise (pitched hiss).
  - <img class="gui-icon" src="{{ '/assets/icons/pulse.png' | relative_url }}" alt=""> **Pulse** — Pulse wave. Width comes from the pulse table.
  - <img class="gui-icon" src="{{ '/assets/icons/saw.png' | relative_url }}" alt=""> **Saw** — Sawtooth.
  - <img class="gui-icon" src="{{ '/assets/icons/triangle.png' | relative_url }}" alt=""> **Triangle** — Triangle.
  - <img class="gui-icon" src="{{ '/assets/icons/test.png' | relative_url }}" alt=""> **Test** — Resets the oscillator (silent while on).
  - <img class="gui-icon" src="{{ '/assets/icons/ring.png' | relative_url }}" alt=""> **Ring** — Ring-modulates with the previous voice (needs **Triangle** on this voice).
  - <img class="gui-icon" src="{{ '/assets/icons/sync.png' | relative_url }}" alt=""> **Sync** — Hard-syncs this oscillator to the previous voice.
  - <img class="gui-icon" src="{{ '/assets/icons/gate.png' | relative_url }}" alt=""> **Gate** — Starts or holds the ADSR envelope. Off begins release.
- **DELAY** — Hold this step for a number of ticks before moving on. Waveform stays as it was.
- **COMMAND** — **EDIT COMMAND** opens the [command editor](#command-editor) and runs that command from the wave table (same idea as a pattern command). Some commands are not available here.
- **RELATIVE** — Offset from the current note. Negative goes down.
- **ABSOLUTE** — Force a fixed pitch.
- **NO CHANGE** — Leave the pitch unchanged.

<p><img src="{{ '/assets/shots/instr-wave-delay.png' | relative_url }}" alt="Wave table row in DELAY mode"></p>

#### Pulse Table

Sets or sweeps the pulse width when the pulse waveform is used.

<p><img src="{{ '/assets/shots/instr-pulse-set.png' | relative_url }}" alt="Pulse table SET PULSE WIDTH"></p>

- **SET PULSE WIDTH** — Jump to an absolute pulse width.

<p><img src="{{ '/assets/shots/instr-pulse-mod.png' | relative_url }}" alt="Pulse table MOD PULSE WIDTH"></p>

- **MOD PULSE WIDTH** — Sweep for **STEPS** ticks at **SPEED** (signed). Positive widens, negative narrows. Chain set and mod rows, then **Loop Here**, for a repeating pulse LFO.

#### Filter Table

A filter table sets routing, cutoff, and cutoff sweeps. The SID has one filter for all three voices, so cutoff, resonance, and passband apply to every voice routed through it. Routing does not follow the voice this instrument plays on. **VOICE 1**, **VOICE 2**, and **VOICE 3** are the chip voices: an instrument on voice 3 is filtered only if **VOICE 3** is enabled. Bundled presets that use the filter enable **VOICE 1** only. Only one filter table runs at a time. A new note on an instrument that has one replaces the filter program already running.

<p><img src="{{ '/assets/shots/instr-filter-params.png' | relative_url }}" alt="Filter table SET PARAMS"></p>

- **SET PARAMS** — Routes voices through the filter and sets the passband and resonance. The passband buttons can be combined.
  - **VOICE 1** / **VOICE 2** / **VOICE 3** — Sends that voice through the filter.
  - <img class="gui-icon" src="{{ '/assets/icons/lowpass.png' | relative_url }}" alt=""> **Lowpass** — Keeps frequencies below the cutoff.
  - <img class="gui-icon" src="{{ '/assets/icons/bandpass.png' | relative_url }}" alt=""> **Bandpass** — Keeps frequencies around the cutoff.
  - <img class="gui-icon" src="{{ '/assets/icons/highpass.png' | relative_url }}" alt=""> **Highpass** — Keeps frequencies above the cutoff.
  - **RES** — Resonance (`0`–`F`). Higher gives a sharper peak at the cutoff.

<p><img src="{{ '/assets/shots/instr-filter-cutoff.png' | relative_url }}" alt="Filter table SET CUTOFF"></p>

- **SET CUTOFF** — Jump to an absolute cutoff. Put this directly under **SET PARAMS** if both should apply on the same tick.

<p><img src="{{ '/assets/shots/instr-filter-mod.png' | relative_url }}" alt="Filter table MOD CUTOFF"></p>

- **MOD CUTOFF** — Sweep cutoff for **STEPS** ticks at **SPEED** (signed), same idea as pulse mod.

### Table Sharing Window

Wave, pulse, and filter tables can each be shared between instruments. Editing a shared table changes it for every instrument that uses it.

<p><img src="{{ '/assets/shots/instr-share.png' | relative_url }}" alt="Table sharing window"></p>

- Tap another instrument to point this instrument at that instrument’s table (and delete this table if nothing else used it). Instruments that have no table are disabled.
- **CLONE** — Copies the table to a new unique block. Disabled unless the table is shared and there is room.
- **DELETE** — Detaches this instrument from the table (and deletes the bytes if this was the last user).
- **CLOSE** — Closes the window.

## Instrument Manager View

Save instruments so you can reuse them across songs, and try the bundled presets to get started quickly. Open with a second tap on **INSTR**. Load and save always apply to the current instrument.

<p><img src="{{ '/assets/shots/instr-manager-files.png' | relative_url }}" alt="Instrument manager files tab"></p>

- **FILES** — Instruments you have saved.
- **PRESETS** — Bundled instruments.
- **File name** (FILES) — Name used when saving.
- **LOAD** — Loads the selected instrument file or preset into the current instrument.
- **SAVE** — Saves the current instrument.
- **DELETE** — Deletes the selected instrument file.

## Settings View

### Project Settings

<p><img src="{{ '/assets/shots/settings-project.png' | relative_url }}" alt="Project settings"></p>

- **CHIP MODEL** — **6581** or **8580**.
- **SPEED** — How often the player ticks. `25Hz` is half-speed. `1X` is once per PAL frame (50 Hz), `2X` twice, and so on. Higher speed means finer timing and denser tables, but also more CPU on a real C64. Changing speed updates the gate timer on empty instruments to `2 ×` speed (the usual default).
- **HARD RESTART** — Opens the hard restart window. Shows the current ADSR used for hard restart before a new note.

#### Hard Restart Window

<p><img src="{{ '/assets/shots/settings-hardrestart.png' | relative_url }}" alt="Hard restart window"></p>

ADSR written during hard restart (the brief gate-off before a note). It mainly shapes how rapid note passages sound.

- **ATTACK** / **DECAY** / **SUSTAIN** / **RELEASE** — Default is soft (`0F00`). Lower values retrigger harder. A little release (for example `0F01`) softens the gate-off phase further. Attack `F` also selects an alternate register write order that can trigger fast releases more reliably.
- **CLOSE** — Closes the window.

### Editor Settings

<p><img src="{{ '/assets/shots/settings-editor.png' | relative_url }}" alt="Editor settings"></p>

- **FULLSCREEN** — Hides the Android status and navigation bars (immersive mode). Swipe from the edge to show them briefly.
- **KEEP SCREEN ON** — Stops the screen from sleeping while the app is open.
- **ROW HEIGHT** / **HIGHLIGHT** — Pattern row size and highlight spacing.
- **REG WRITE ORDER** — SID register write order **v2.68** or **v2.73**.
- **SAMPLING METHOD** — Opens the sampling-method window.

#### Sampling Method Window

<p><img src="{{ '/assets/shots/settings-sampling.png' | relative_url }}" alt="Sampling method window"></p>

- **FAST**, **INTERPOLATE**, **RESAMPLE INTERPOLATE**, **RESAMPLE FAST** — Picks reSID sampling. The choice applies on tap.
- **CLOSE** — Closes the window.

## Differences from GoatTracker 2

GTMobile songs are still `.sng` files and play in GoatTracker 2. The file format matches. These are the places the song data is organized differently:

- **Tables are per instrument** — The song still stores GoatTracker’s packed wave, pulse, filter, and speed tables. GTMobile assigns the pointers. You edit wave, pulse, and filter on the instrument that uses them, not as raw row addresses.
- **Speed table** — That packed speed table is split into portamento, vibrato, and funk-tempo ranges. Instrument vibrato uses the vibrato range. Pattern commands pick a slot in the matching range, not a raw speed-table address.
- **Table pointer commands** — In GoatTracker, commands `8`, `9`, and `A` store a table address. In GTMobile the data byte is an instrument number (or `00` for off). Playback uses that instrument’s wave, pulse, or filter pointer.
- **Shared tables** — Several instruments can point at the same table bytes. Sharing, cloning, or dropping that link is how a table is reused or removed. The packed bytes stay hidden.
- **Order list** — All three voices share one order-list length, and patterns that play on the same row should have the same length. The loop start is a mark on an order row.
