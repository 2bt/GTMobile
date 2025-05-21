---
layout: page
title: Manual
permalink: /manual/
---


## Introduction

GTMobile is designed to make **creating C64 music more accessible**.
While it provides a mobile-friendly tracker experience,
it may take some time to get familiar with,
especially if you are new to the **SID chip** or **trackers** in general.


## App Layout and Navigation

GTMobile’s interface consists of two fixed sections and a main workspace.
At the bottom, a piano keyboard allows you to play notes,
with additional buttons for instrument selection and playback control.
At the top, a row of four tab buttons lets you switch between views,
each focusing on a different aspect of song creation.

+ **PROJECT** – Manage song files, edit song metadata.
+ **SONG** – Edit and arrange patterns.
+ **INSTRUMENT** – Edit, and manage instruments. Adjust wave, pulse, and filter tables.
+ **SETTINGS** – Configure editor preferences and project settings.

In the following chapters, we will explore each view in more detail.


## Project View

The **PROJECT** view is the default view when you start the app.
Here, you can view and edit the song's metadata, reset the song data, manage song files, and import and export songs.
Note that in GTMobile, you edit exactly one song at a time.

<img src="{{ '/assets/project.png' | relative_url }}">

The view shows you some text input fields with the song list below, and a number of buttons to the right
Let's go through each.

+ **TITLE**, **AUTHOR**, and **RELEASED** are song metadata which you can set as you please.
+ **FILE** specifies the name for the current song file. The buttons **LOAD**, **DELETE**, and **SAVE** operate on this file.
+ The song list shows all available song files.
  Note that GTMobile comes with two demo songs.
  Selecting a song from the song list puts the name in the **FILE** input field.
+ **RESET** resets the current song data.
+ **LOAD** loads the selected song from the song list.
+ **SAVE** saves the current song under the name in the input field.
Change the file name before saving to create a new file.
+ **DELETE** deletes the selected song from the song list.
+ **IMPORT** lets you import a song file.
+ **EXPORT** opens an export dialog window where you can export the song file directly, or render to **WAV** or **OGG**.

<p>
    <img src="{{ '/assets/export.png' | relative_url }}">
    <em>The export dialog window.</em>
</p>

Currently, GTMobile lacks direct **SID** export.
To get a **SID** file of your song, export the song to **SNG** and convert it to **SID** in GoatTracker.


## Song View

A song consists of **instruments** and **patters**.

**Instruments** define how notes are played on the SID chip,
with settings for the volume envelope, waveform, filter, and other sound parameters.
A song may use up to 63 instruments.
Instruments are referenced by a hexadecimal number ranging from `01` to `3F`.

**Patterns** are sequences of musical instructions, containing notes, instrument references, and commands.
A song may use up to 208 patterns.
Patterns are referenced by a hexadecimal number ranging from `00` to `CF`.

The **SONG** view allows you to edit and arrange patterns. It shows the **song table** at the top, the **pattern table** below, and a column of various buttons to the right. What buttons are shown is context-dependent.

<img src="{{ '/assets/song2.png' | relative_url }}">

The **song table** manages the song structure and contains patterns references.
It specifies the order in which patterns are played on each of the SID chip's three voices.
Within the table, patterns can be **transposed**, allowing them to be reused at different pitches.

The **pattern table** ...

TODO

## Instrument View


When the app starts, a very basic song is loaded with one simple instrument defined.

TODO


## Settings View

TODO


<!-- ## Differences to GoatTracker 2 -->
