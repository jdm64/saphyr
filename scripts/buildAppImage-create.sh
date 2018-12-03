#!/usr/bin/env bash

AppImageToolBin=$(mktemp)
AppImageVer=11

wget https://github.com/AppImage/AppImageKit/releases/download/$AppImageVer/appimagetool-x86_64.AppImage -O $AppImageToolBin
chmod +x $AppImageToolBin

cd AppImage && ln -s usr/bin/saphyr AppRun && cd ..

$AppImageToolBin -n --comp xz AppImage
