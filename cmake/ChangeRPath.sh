#!/bin/bash

if [ ! -f "platforms/libqxcb.so" ]
then
	echo "ChangeRPath.sh: platforms/libqxcb.so not found!"
else
	patchelf --set-rpath '$ORIGIN/..' platforms/libqxcb.so
fi

