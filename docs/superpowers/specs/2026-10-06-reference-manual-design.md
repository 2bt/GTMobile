# GTMobile reference manual

## Goal

Replace the incomplete Jekyll Manual with a **button-level reference** on the existing `/manual/` page. Source code is the source of truth. A later chapter on the same page documents where GTMobile differs from GoatTracker 2. The written intro stays short; this page is a control reference, not a tutorial.

## Out of scope

- A beginner tutorial (written or video). Do not leave a video placeholder.

- A full GoatTracker opcode bible (command/table *values*). Document the editors and buttons that open them, not every legal byte.
- Automated UI driving or screenshot capture.
- Changing app behavior.

## Site

- **One page:** `docs/manual.markdown`, permalink `/manual/`, already linked from the header and home page.
- Keep History and Privacy as they are.
- Intro: 1–2 short paragraphs (GoatTracker 2–based, one song at a time, familiar workflow). No video section.
- Fix stale facts while rewriting (in particular: SID export exists; do not claim it is missing).
- Remove references to image files that are not in the repo (`export.png`, `song-cell-buttons.png`, and similar). Replace with the new shot set.

## Page structure (app order)

Each **mode** is: one full-canvas screenshot, then a list of visible controls. Gestures that *change* the mode are listed with that mode (tap, long-press, drag).

Repeated actions (Copy, Paste, Close, Confirm OK/Cancel) get a full sentence **once**; later occurrences use a short “same as …” pointer.

1. Intro
2. **Always-on controls** — tabs `PROJECT` / `SONG` / `INSTR` / Settings; undo/redo; transport (fast back, stop, play/pause, follow, loop, fast forward). State that `INSTR` toggles instrument editor vs instrument manager (second tap).
3. **Piano** — keys, octave scrollbar, instrument button, long-press in Song view copies the selected pattern row’s instrument, instrument picker window.
4. **Project** — metadata fields, FILES/DEMOS, file list, LOAD/SAVE/DELETE/IMPORT/EXPORT/RESET, export window (SNG/SID/WAV/OGG), confirm/alert.
5. **Song**
   - Order list: tap a cell vs long-press+drag region.
   - Pattern channels: mute/solo-style channel buttons with meters.
   - Pattern list: tap a row vs region.
   - Windows: pattern index (order edit), pattern length, auto-step, command editor.
6. **Instrument**
   - Header: number, name, prev/next, copy/paste.
   - Selecting ADSR / vibrato / gate timer / 1st wave (each swaps the bottom editor).
   - WAVE / PULSE / FILTER tabs, share button, table row tools (when a table row is selected).
   - Table-row editor: one shot per *editor-strip kind* (wave / delay / command; set vs mod pulse; filter params / cutoff / mod), not every opcode.
   - Share window; then **Instrument manager** (FILES/PRESETS).
7. **Settings** — Project vs Editor tabs; hard-restart and sampling-method windows.
8. **Differences from GoatTracker 2** (pass 2).

Splash (tap to enter Project) is one sentence under Intro; no required screenshot.

## Control list format

For each control:

- **Icon buttons:** 16×16 PNG from the atlas, short English name matching the `Icon` enum (`Copy`, `Play`, `Add row above`, …), one present-tense sentence of what the handler does.
- **Text buttons:** on-screen label (`LOAD`, `CLOSE`) with no glyph.
- **Custom font glyphs** (gate off/on, transpose arrows): name them in English (`Gate off/on`, `Transpose up`); no atlas extract. Do not invent extra icon files.
- Long-press: same bullet as the tap action.
- Disabled state: mention only if it changes meaning (e.g. Paste with empty buffer still pastes empty).

Copy is written from the `gui::button(...)` handler in:

- `src/app.cpp` — tabs, undo/redo, transport, confirm
- `src/piano.cpp`
- `src/project_view.cpp`
- `src/song_view.cpp`
- `src/command_edit.cpp`
- `src/instrument_view.cpp`
- `src/instrument_manager_view.cpp`
- `src/settings_view.cpp`

## Icons

`assets/gui.png` is a 16×16 tile atlas. `DrawContext::icon` maps `int(Icon)` to UV `(id % 16 * 16, id / 16 * 16)`.

Add `docs/extract-icons.py` that:

1. Parses `enum class Icon` in `src/gui.hpp` (explicit values and implicit continuation; skip blank lines).
2. Crops each named icon from `assets/gui.png`.
3. Writes `docs/assets/icons/<lowercase-enum-name>.png` (`copy.png`, `playpause.png`, …).

Run when the enum or atlas changes. Commit the PNGs so the site does not need the script at build time.

CSS in `docs/assets/main.scss`:

```scss
.gui-icon {
  width: 16px;
  height: 16px;
  image-rendering: pixelated;
  vertical-align: middle;
}
```

Markdown usage: `<img class="gui-icon" src="{{ '/assets/icons/copy.png' | relative_url }}" alt=""> **Copy** — …`

## Screenshots

Desktop Print Screen writes `shot-NNNN.ppm` of the **canvas** (not the OS window frame). Convert to PNG; do not commit PPMs.

- Raw drops: `docs/shots-raw/` (gitignored).
- Published: `docs/assets/shots/<id>.png`.
- Convert: ImageMagick `convert docs/shots-raw/shot-NNNN.ppm docs/assets/shots/<id>.png` (or `magick`).

**Who:** the author captures; the writer converts, names, and captions from source. The agent cannot navigate the SDL/GL UI.

**Song data:** load demo **Brite Byte** before Song/Instrument/Command shots so tables and patterns are not empty.

If a mode is too awkward to capture, skip the image and still write the control list; add the file later without blocking the chapter.

### Shot checklist

Map each capture to `docs/assets/shots/<id>.png`. Print Screen once per row after the UI matches **Setup**.

| id | Done | Setup |
|---|---|---|
| `always-on` | ✓ | Tabs, undo/redo, and transport; middle of the canvas faded out. |
| `piano-select` | ✓ | Instrument picker open. |
| `project-files` | ✓ | Project, FILES tab. |
| `project-demos` | ✓ | Project, DEMOS tab, Brite Byte selected. |
| `project-export` | ✓ | Export window open (SNG/SID/WAV/OGG). |
| `project-confirm` | | Unused — confirm dialogs are not documented separately. |
| `song-order-cell` | ✓ | Song view; one order-list cell selected. |
| `song-order-region` | ✓ | Order-list region selected (long-press+drag). |
| `song-order-edit` | ✓ | Pattern index window open. |
| `song-pattern-cell` | ✓ | One pattern row selected. |
| `song-pattern-region` | ✓ | Pattern region selected. |
| `song-pattern-length` | ✓ | Pattern length window open. |
| `song-autostep` | ✓ | Auto-step window open (long-press auto-step). |
| `song-command-porta` | ✓ | Command editor: portamento (up/down/tone). Crop to window. |
| `song-command-vibrato` | | Command editor: vibrato. Crop to window. |
| `song-command-ad` | ✓ | Command editor: attack/decay (sustain/release is the same layout). Crop to window. |
| `song-command-wave` | ✓ | Command editor: wave flags. Crop to window. |
| `song-command-table` | ✓ | Command editor: wave/pulse/filter table pointer. Crop to window. |
| `song-command-volume` | ✓ | Command editor: master volume. Crop to window. |
| `song-command-funk` | ✓ | Command editor: funk tempo. Crop to window. |
| `song-command-tempo` | ✓ | Command editor: tempo. Crop to window. |
| `song-command-filter-control` | ✓ | Command editor: filter control (voices + resonance). Crop to window. |
| `song-command-filter-cutoff` | ✓ | Command editor: filter cutoff. Crop to window. |
| `instr-header` | | Unused — covered by `instr-adsr` (ADSR is the default selection). |
| `instr-adsr` | ✓ | Instrument overview / ADSR selected (four sliders). Also used as section intro. |
| `instr-vibrato` | ✓ | Vibrato field selected. |
| `instr-gatetimer` | ✓ | Gate timer selected. |
| `instr-firstwave` | ✓ | 1st wave selected, WAVE mode (waveform icons visible). |
| `instr-table-tools` | | Unused — covered by `instr-wave-wave` (row tools show whenever a table row is selected). |
| `instr-wave-wave` | ✓ | Wave table row, WAVE mode (row tools + waveform/note editor; absolute/relative covered in text). |
| `instr-wave-delay` | ✓ | Wave table row, DELAY mode. |
| `instr-wave-command` | | Unused for now — EDIT COMMAND opens the command editor already documented above. |
| `instr-pulse-set` | ✓ | Pulse table, SET PULSE WIDTH. |
| `instr-pulse-mod` | ✓ | Pulse table, MOD PULSE WIDTH. |
| `instr-filter-params` | ✓ | Filter table, SET PARAMS (voice + LP/BP/HP). |
| `instr-filter-cutoff` | ✓ | Filter table, SET CUTOFF. |
| `instr-filter-mod` | ✓ | Filter table, MOD CUTOFF. |
| `instr-share` | ✓ | Table sharing window open. Crop to window. |
| `instr-manager-files` | ✓ | INSTR second tap; FILES tab. |
| `instr-manager-presets` | | Unused — FILES tab shot covers the manager. |
| `settings-project` | ✓ | Settings, PROJECT SETTINGS. |
| `settings-editor` | ✓ | Settings, EDITOR SETTINGS. |
| `settings-hardrestart` | ✓ | Hard restart window. Crop to window. |
| `settings-sampling` | ✓ | Sampling method window. Crop to window. |

Popup shots are cropped to the window with `docs/crop-window.py` (frame-color bbox). Full-canvas PPMs stay in `docs/shots-raw/`.

## Pass 2 — differences from GoatTracker 2

Same page, after the UI reference. Short sections, not a button list. Seed (expand from code while writing; drop anything that is not actually different):

- **No global table view.** WAVE/PULSE/FILTER are per instrument, not GoatTracker’s shared table pages with raw addresses.
- **Speed table.** STBL is split internally (`STBL_PORTA_START`, `STBL_VIB_START`, `STBL_FUNK_START` in `gtsong.hpp`). The UI never shows a global speed table. Vibrato lives on the instrument; porta/vibrato/funk tempo are picked from sub-lists in the command editor.
- **Table-pointer commands (8/9/A).** Data selects an **instrument** (use that instrument’s wave/pulse/filter pointer), not a table row address. `0` is off. Long-press Edit on such a pattern command jumps to that instrument and table.
- **Table sharing.** Share / clone / delete (`Icon::Share` and the sharing window) instead of editing packed global tables.
- **Order list.** Transpose and loop as in this app’s order editor (loop row marker, JumpBack sets loop, clone pattern).
- **Mobile-only controls.** Piano, follow, record, auto-step, instrument manager, no GoatTracker keyboard/menu map.
- **Export.** SNG, SID, WAV, OGG from the export window.

Do not restate the whole UI here; only behavior a GoatTracker user would get wrong.

## Implementation order

1. Icon extract script + CSS + empty/short intro skeleton on `docs/manual.markdown`.
2. Pass 1 chapters as shots arrive (always-on controls → piano → project → song → instrument → settings). A chapter may ship with shot files missing; lists must still be complete from source.
3. Pass 2 differences chapter.
4. Check the Jekyll page (`docs/s.sh` / `bundle exec jekyll serve`) for broken images and leftover TODOs.

`todo.md` “differences to GoatTracker2” can be marked done when pass 2 lands.

## Constraints

- No new documentation toolchain (no Docusaurus, no generated button scrape).
- No PPM or `shot-*.ppm` in git.
- Do not change application code except if Print Screen or atlas layout is broken (not expected).
- English, brief, present tense.
