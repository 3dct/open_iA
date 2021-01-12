#!/bin/bash

# patch all shared objects to have an RPATH of '$ORIGIN':
for lib in *.so*
do
	# echo "Patching runpath in $lib"
	if [ ! -f "$lib" ]
	then
		echo "ChangeRPath.sh: $lib not found!"
	else
		patchelf --set-rpath '$ORIGIN' $lib
	fi
done

for lib in plugins/*.so
do
	patchelf --set-rpath '$ORIGIN/..' ${lib}
	# Remove any plugin libraries copied directly to root folder because of dependencies:
	rootpluglib=$(basename $lib)
	rm -f $rootpluglib
done

declare -a libarray=("iconengines/libqsvgicon.so" "imageformats/libqsvg.so" "platforms/libqxcb.so" "xcbglintegrations/libqxcb-glx-integration.so" "xcbglintegrations/libqxcb-egl-integration.so")

for lib in ${libarray[@]}
do
	if [ ! -f "${lib}" ]
	then
		echo "ChangeRPath.sh: ${lib} not found!"
	else
		#echo "Patching RPATH in $lib"
		patchelf --set-rpath '$ORIGIN/..' ${lib}
	fi
done

