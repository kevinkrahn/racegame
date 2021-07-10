#!/bin/bash

export GNOME_DISABLE_CRASH_DIALOG=TRUE
#./build.py
TIMEFORMAT=%R
time make debug
cd bin
./game
