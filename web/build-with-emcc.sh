#!/bin/bash

source /home/dlangner/Programming/c++/emsdk/emsdk_env.sh

emcc \
    -s USE_SDL=2 -s USE_WEBGL2=1 -Os \
    -s TOTAL_STACK=1MB \
    -s INITIAL_MEMORY=32MB \
    -s ALLOW_MEMORY_GROWTH=1 \
    --shell-file shell.html \
    --preload-file ../assets \
    ../src/platform.cpp \
    ../src/gfx.cpp \
    ../src/app.cpp \
    ../src/command_edit.cpp \
    ../src/gtplayer.cpp \
    ../src/gtsong.cpp \
    ../src/gui.cpp \
    ../src/instrument_view.cpp \
    ../src/piano.cpp \
    ../src/project_view.cpp \
    ../src/settings_view.cpp \
    ../src/sid.cpp \
    ../src/song_view.cpp \
    -o index.html
