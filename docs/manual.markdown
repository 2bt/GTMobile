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


## Song Basics

A song consists of **patters** and **instruments**.
+ **Patterns** are sequences of musical instructions, containing notes, instrument references, and commands.
+ **Instruments** define how notes are played on the SID chip,
with settings for the volume envelope, waveform, filter, and other sound parameters.

The song structure is managed through a table of pattern references,
specifying the order in which patterns are played on each of the SID chip's three voices.
Patterns can be **transposed** within the table, allowing them to be reused at different pitches.


## App Layout and Navigation

GTMobile’s interface consists of two fixed sections and a main workspace.
At the bottom, a piano keyboard allows you to play play notes,
with additional buttons for instrument selection and playback control.
At the top, a row of four tab buttons lets you switch between views,
each focusing on a different aspect of song creation.

+ **PROJECT** – Load and save song files, and edit song metadata.
+ **SONG** – Edit and arrange patterns.
+ **INSTRUMENT** – Edit, load, and save instruments. Adjust wave, pulse, and filter tables.
+ **SETTINGS** – Configure editor preferences and project settings.

In the following chapters, we will explore each view in more detail.


## Project View

<img src="{{ '/assets/project.png' | relative_url }}">

**TODO**

The **PROJECT** view is the default view when you start the app, after the splash screen.
Here, you can:
+ set the song's metadata, namely the **TITLE**, **AUTHOR**, and **RELEASED** note of the song you are
+ reset the song data of the current song
+ load, save, and delete song files
+ export songs



Note that in GTMobile, you edit exactly one song at a time.

To load a previously saved song, simply select a song from the song list.
This will enter the song name in the song name input field.
Now press **LOAD**.
Press **SAVE** to save the current song under the name in the input field.
An existing song file of that name will be overwritten.
Change the file name before saving to create a new file.
Press **DELETE** to delete the selected song.
Press **RESET** to reset the current song.




You may render the current song to **WAV** or **OGG**
by first selecting the desired file format and then pressing **EXPORT**.


<div class="note">
  {% capture content %}
  Where are the song files stored?

  TODO: print the path here.
  {% endcapture %}
  {{ content | markdownify }}
</div>

<div class="note">
Can I export SID files?
GTMobile lacks direct SID export.
But you can export the SNG file and convert it to SID in GoatTracker.
</div>



TODO


## Song View


When the app starts, a very basic song is loaded with just one instrument **Beep** defined.


TODO


## Instrument View

TODO


## Settings View

TODO


<!-- ## Differences to GoatTracker 2 -->
