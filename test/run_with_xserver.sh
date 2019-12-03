#!/bin/bash

export DISPLAY=:99.0
export SCREEN_WIDTH=1600
export SCREEN_HEIGHT=1200
export SCREEN_DEPTH=24
export GEOMETRY="${SCREEN_WIDTH}x${SCREEN_HEIGHT}x${SCREEN_DEPTH}"

# use short-hand -e instead of --error-file.
#     This is because centOS 6 is affected by this bug:
#      https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=337703;msg=2
xvfb-run -e /workspace/xvfb.log --server-args="$DISPLAY -screen 0 $GEOMETRY -ac +extension RANDR" "$@"
