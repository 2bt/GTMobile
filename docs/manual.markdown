---
layout: page
title: Manual
permalink: /manual/
---


GTMobile is designed to make **composing C64 music more accessible**.
While it provides a mobile-friendly tracker experience,
it may take some time to get familiar with,
especially if you are new to the **SID chip** or **trackers** in general.


## App Layout and Navigation

GTMobile’s interface consists of two fixed sections and a main workspace.
At the bottom, a piano keyboard allows you to play notes,
with additional buttons for instrument selection and playback control.
At the top, a row of four tab buttons lets you switch between views,
each focusing on a different aspect of song creation:

+ **PROJECT** – Manage song files, edit song metadata.
+ **SONG** – Edit and arrange patterns.
+ **INSTRUMENT** – Edit and manage instruments.
+ **SETTINGS** – Configure editor preferences and project settings.

In the following chapters, we will explore each view in more detail.


## Project View

The **PROJECT** view is the default view when you start the app.
Here, you can view and edit song metadata and manage song files.
Note that in GTMobile, you edit exactly one song at a time.

<img src="{{ '/assets/project.png' | relative_url }}">

At the top, there are the following text input fields:

+ **TITLE** – The song title.
+ **AUTHOR** – The song author.
+ **RELEASED** A copyright note.
+ **FILE** – The name for the current song file. The buttons **LOAD**, **DELETE**, and **SAVE** operate on this file.

The song list below these text input fields shows all available song files.
Note that GTMobile comes with two demo songs.
Selecting a song from the song list also sets the name in the **FILE** input field accordingly.

To the right of the table, there is a column of buttons:

+ **RESET** – Reset the current song data.
+ **LOAD** – Load the selected song from the song list.
+ **SAVE** – Save the current song under the name in the input field. Change the file name before saving to create a new file.
+ **DELETE** – Delete the selected song from the song list.
+ **IMPORT** – Import a song file.
+ **EXPORT** – Open the **export window** where you can export the song file directly or render to **WAV** or **OGG**.

<p>
    <img src="{{ '/assets/export.png' | relative_url }}">
    <em>The export window</em>
</p>

Currently, GTMobile lacks direct **SID** export.
To get a **SID** file of your song, export the song to **SNG** and convert it to **SID** in GoatTracker.


## Song View

A song consists of **instruments** and **patters**.

**Instruments** define how notes are played on the SID chip,
with settings for the volume envelope, waveform, filter, and other sound parameters.
A song may use up to 63 instruments.
Instruments are indexed by a hexadecimal number ranging from `01` to `3F`.

**Patterns** are sequences of musical instructions, containing notes, instrument indices, and commands.
A song may use up to 208 patterns.
Patterns are indexed by a hexadecimal number ranging from `00` to `CF`.

The **SONG** view allows you to edit and arrange patterns. It shows the **song table** at the top, the **pattern table** below, and a column of various buttons to the right. What buttons are shown is context-dependent.

<img src="{{ '/assets/song2.png' | relative_url }}">

### Song Table

The **song table** manages the song structure and contains pattern indices.
It specifies the order in which patterns are played on each of the SID chip's three voices.
Within the table, patterns can be **transposed**, allowing them to be reused at different pitches.

**Tap** a cell to select it. The entire row becomes activate, updating updating the pattern table to show that rows three patterns.
**Long-press + drag** to select a multiple cells (rows and/or columns).

The following buttons are available when a single cell is selected:

<img src="{{ '/assets/song-cell-buttons.png' | relative_url }}">

+ **PASTE** – Paste the copied region starting at the selected cell.
+ **ADD ROW ABOVE** – Insert a new row above the current one.
+ **ADD ROW BELOW** – Insert a new row below the current one.
+ **DELETE ROW** – Delete the selected row.
+ **LOOP** – Mark the selected row as the loop start point when playback restarts.
+ **EDIT** – Open the **pattern index window** for the selected cell.
<p>
    <img src="{{ '/assets/set-pattern.png' | relative_url }}">
    <em>The pattern index window. Note that empty patterns are shown with a darker shade.</em>
</p>

The following buttons are available when a multiple cells are selected:

<img src="{{ '/assets/song-region-buttons.png' | relative_url }}">

+ **COPY** – Copy the selected region.
+ **TRANSPOSE UP** – Transpose selected region up by one semitone.
+ **TRANSPOSE DOWN** – Transpose selected region down by one semitone.

### Pattern Table

The **pattern table** displays the three patterns as referenced by the active song table row.
The three buttons above the table show each pattern's index and let you mute/unmute that SID voice.

A pattern comprises multiple rows and each row may contain a note with an instrument index and a pattern command.

**Tap** a pattern row to select it.
**Long-press + drag** to select a region of pattern rows spanning one or more patterns.

When a single pattern row is selected, the following buttons are available, grouped into sections:

<img src="{{ '/assets/pattern-cell-buttons.png' | relative_url }}">

1. General Operations
  + **PASTE** – Paste the copied region starting at the selected row.
  + **PATTERN LENGTH** – Open the **pattern length window**.
2. Note & Instrument Controls
  + **RECORD** – Enable/disable note recording.
    When enabled, tapping the piano writes the note and current instrument to this row.
  + **GATE OFF/ON** – Write a **gate-off** event. If the row already has **gate-off**, replace it with **gate-on**.
  + **CLEAR NOTE** – Clear the note and instrument index of this row.
3. Command Controls
  + **EDIT COMMAND** – Open the **command editor** window for this row.
    If a table command is selected (i.e., `8`, `9`, or, `A`),
    **long-pressing** this button jumps to **INSTRUMENT** view, showing the corresponding instrument and table.
  + **CLEAR COMMAND** – Delete an existing command in this row.
4. Playback
  + **PLAY** – Play the song starting at the selected pattern row.

When multiple pattern rows are selected, a number of buttons are available, grouped into sections:

<img src="{{ '/assets/pattern-region-buttons.png' | relative_url }}">

1. General Operations
  + **COPY** – Copy the region.
  + **CLEAR** – Clear the region.
2. Note & Instrument Controls
  + **TRANSPOSE UP** – Raise selected notes by one semitone.
  + **TRANSPOSE DOWN** – Lower selected notes by one semitone.
  + **INSTRUMENT OVERWRITE** – Set all selected notes to the current instrument.
  + **COPY NOTE** – Copy notes from the region.
  + **CLEAR NOTE** – Clear notes of the region.
3. Command Controls
  + **EDIT COMMAND** – Open the **command editor** window for the entire region.
  + **COPY COMMAND** – Copy commands from the region.
  + **CLEAR COMMAND** – Clear commands of the region.


### Pattern Length Window

<img src="{{ '/assets/pattern-dialog.png' | relative_url }}">



### Piano Keyboard

Although available in all views, the **piano keyboard** is particularly useful while in **SONG** view.

<img src="{{ '/assets/piano.png' | relative_url }}">

TODO

**Long-press** the **intrument select** button to select the instrument of the currently selected pattern row.

<img src="{{ '/assets/select-instrument.png' | relative_url }}">

TODO

### Command Editor

TODO






## Instrument View


When the app starts, a very basic song is loaded with one simple instrument defined.

TODO


## Settings View

TODO


<!-- ## Differences to GoatTracker 2 -->
