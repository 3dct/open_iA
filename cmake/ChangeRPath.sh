#!/bin/bash

# echo "Patching all .so files in $(pwd)"
# for file in *.so*; do patchelf --set-rpath '$ORIGIN' $file; done
for file in $(find . -name '*.so*')
do
	# echo "Patching runpath in $file"
	patchelf --set-rpath '$ORIGIN' $file
done
