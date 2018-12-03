#!/usr/bin/env bash

# setup dirs
mkdir -p AppImage/usr/bin AppImage/usr/lib

# copy libs
cp `ldd ../saphyr | cut -d' ' -f3 | grep boost` AppImage/usr/lib/
cp `ldd ../saphyr | cut -d' ' -f3 | grep LLVM` AppImage/usr/lib/
cp `ldd ../saphyr | cut -d' ' -f3 | grep libedit` AppImage/usr/lib/

# copy & strip binary and setup symlink to AppRun
cp ../saphyr AppImage/usr/bin/
strip -s AppImage/usr/bin/saphyr
cd AppImage && ln -s usr/bin/saphyr AppRun && cd ..

# patch binary so libraries load correctly
patchelf --set-rpath '$ORIGIN/../lib' AppImage/usr/bin/saphyr
