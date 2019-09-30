#!/bin/bash

export DISPLAY=:99.0
export SCREEN_WIDTH=1600
export SCREEN_HEIGHT=1200
export SCREEN_DEPTH=24
export GEOMETRY="${SCREEN_WIDTH}x${SCREEN_HEIGHT}x${SCREEN_DEPTH}"

#unrecognized option +extensions"
# 
xvfb-run --error-file=/workspace/xvfb.log --server-args="$DISPLAY -screen 0 $GEOMETRY -ac +extension RANDR" "$@"
