#!/usr/bin/env bash

# exit on errors
set -e

# setup dirs
rm -rf AppImage/AppRun AppImage/usr
mkdir -p AppImage/usr/bin AppImage/usr/lib

# copy libs
cp `ldd ../saphyr | cut -d' ' -f3 | grep -e boost -e LLVM -e libedit` AppImage/usr/lib/

# copy & strip binary and setup symlink to AppRun
cp ../saphyr AppImage/usr/bin/
strip -s AppImage/usr/bin/saphyr
cd AppImage && ln -s usr/bin/saphyr AppRun && cd ..

# patch binary so libraries load correctly
patchelf --set-rpath '$ORIGIN/../lib' AppImage/usr/bin/saphyr

AppImageToolBin=$(mktemp)
AppImageVer=11

wget https://github.com/AppImage/AppImageKit/releases/download/$AppImageVer/appimagetool-x86_64.AppImage -O $AppImageToolBin
chmod +x $AppImageToolBin

$AppImageToolBin -n --comp xz AppImage
