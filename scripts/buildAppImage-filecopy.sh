#!/usr/bin/env bash

mkdir -p AppImage/usr/bin AppImage/usr/lib
cp `ldd ../saphyr | cut -d' ' -f3 | grep boost` AppImage/usr/lib/
cp `ldd ../saphyr | cut -d' ' -f3 | grep LLVM` AppImage/usr/lib/
cp `ldd ../saphyr | cut -d' ' -f3 | grep libedit` AppImage/usr/lib/
cp ../saphyr AppImage/usr/bin/
strip -s AppImage/usr/bin/saphyr
