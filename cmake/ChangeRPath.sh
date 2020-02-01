#!/bin/bash

# patch all shared objects to have an RPATH of '$ORIGIN':
for file in *.so*
do
	# echo "Patching runpath in $file"
	if [ ! -f "$file" ]
	then
		echo "ChangeRPath.sh: $file not found!"
	else
		patchelf --set-rpath '$ORIGIN' $file
	fi
done


if [ ! -f "platforms/libqxcb.so" ]
then
	echo "ChangeRPath.sh: platforms/libqxcb.so not found!"
else
	patchelf --set-rpath '$ORIGIN/..' platforms/libqxcb.so
fi

