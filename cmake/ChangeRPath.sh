#!/bin/bash

# patch all shared objects to have an RPATH of '$ORIGIN':
for file in *.so*
do
	# echo "Patching runpath in $file"
	if [ ! -f "$file" ]
	then
		echo "ChangeRPath.sh: $file not found!"
	else
		echo "Patching RPATH in $file"
		patchelf --set-rpath '$ORIGIN' $file
	fi
done

declare -a libarray=("iconengines/libqsvgicon.so" "imageformats/libqsvg.so" "platforms/libqxcb.so" "xcbglintegrations/libqxcb-glx-integration.so" "xcbglintegrations/libqxcb-egl-integration.so")

for lib in ${libarray[@]}
do
	if [ ! -f "${lib}" ]
	then
		echo "ChangeRPath.sh: ${lib} not found!"
	else
		echo "Patching RPATH in $lib"
		patchelf --set-rpath '$ORIGIN/..' ${lib}
	fi
done

