#!/bin/bash

# echo "Patching all .so files in $(pwd)"
# for file in *.so*; do patchelf --set-rpath '$ORIGIN' $file; done
for file in *.so*
do
	# echo "Patching runpath in $file"
	patchelf --set-rpath '$ORIGIN' $file
done

patchelf --set-rpath '$ORIGIN/..' platforms/libqxcb.so
