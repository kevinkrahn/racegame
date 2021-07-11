#!/bin/bash

set -e

export GNOME_DISABLE_CRASH_DIALOG=TRUE
#./build.py
TIMEFORMAT=%R
time make debug -j4
cd bin
./game
